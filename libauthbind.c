/*
 *  libauthbind.c - bind(2)-redirector library for authbind
 *
 *  authbind is Copyright (C) 1998 Ian Jackson
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 * 
 */

#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

static const char *rcsid="$Id$";

#include "authbind.h"

typedef void anyfn_type(void);
typedef int bindfn_type(int fd, const struct sockaddr *addr, socklen_t addrlen);

#define STDERRSTR_CONST(m) write(2,m,sizeof(m)-1)
#define STDERRSTR_STRING(m) write(2,m,strlen(m))

static int find_any(const char *name, anyfn_type **keep) {
  static const char *dlerr;
  anyfn_type *kv;

  if (*keep) return 0;
  kv= dlsym(RTLD_NEXT,name);
  if (kv) { *keep= kv; return 0; }
  dlerr= dlerror(); if (!dlerr) dlerr= "dlsym() failed for no reason";
  STDERRSTR_CONST("libauthbind: error finding original version of ");
  STDERRSTR_STRING(name);
  STDERRSTR_CONST(": ");
  STDERRSTR_STRING(dlerr);
  STDERRSTR_STRING("\n");
  errno= ENOSYS;
  return -1;
}

static bindfn_type find_bind, *old_bind= find_bind;

int find_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
  anyfn_type *anyfn;
  if (find_any("bind",&anyfn)) return -1;
  old_bind= (bindfn_type*)anyfn;
  return old_bind(fd,addr,addrlen);
}

static int exiterrno(int e) {
  _exit(e>0 && e<128 ? e : -1);
}

static void removepreload(void) {
  const char *myself, *found;
  char *newval, *preload;
  int lpreload, lmyself, before, after;

  preload= getenv(PRELOAD_VAR);
  myself= getenv(AUTHBINDLIB_VAR);
  if (!myself || !preload) return;

  lpreload= strlen(preload);
  lmyself= strlen(myself);

  if (lmyself < 1 || lpreload<lmyself) return;
  if (lpreload==lmyself) {
    if (!strcmp(preload,myself)) unsetenv(PRELOAD_VAR);
    return;
  }
  if (!memcmp(preload,myself,lmyself) && preload[lmyself]==':') {
    before= 0; after= lpreload-(lmyself+1);
  } else if (!memcmp(preload+lpreload-lmyself,myself,lmyself) &&
	     preload[lpreload-(lmyself+1)]==':') {
    before= lpreload-(lmyself+1); after= 0;
  } else {
    if (lpreload<lmyself+2) return;
    found= preload+1;
    for (;;) {
      found= strstr(found,myself); if (!found) return;
      if (found > preload+lpreload-(lmyself+1)) return;
      if (found[-1]==':' && found[lmyself]==':') break;
      found++;
    }
    before= found-preload;
    after= lpreload-(before+lmyself+1);
  }
  newval= malloc(before+after+1);
  if (newval) {
    memcpy(newval,preload,before);
    strcpy(newval+before,preload+lpreload-after);
    if (setenv(PRELOAD_VAR,newval,1)) return;
    free(newval);
  }
  strcpy(preload+before,preload+lpreload-after);
  return;
}

int _init(void);
int _init(void) {
  char *levels;
  int levelno;

  /* If AUTHBIND_LEVELS is
   *  unset => always strip from preload
   *  set and starts with `y' => never strip from preload, keep AUTHBIND_LEVELS
   *  set to integer > 1 => do not strip now, subtract one from AUTHBIND_LEVELS
   *  set to integer 1 => do not strip now, unset AUTHBIND_LEVELS
   *  set to empty string or 0 => strip now, unset AUTHBIND_LEVELS
   */
  levels= getenv(AUTHBIND_LEVELS_VAR);
  if (levels) {
    if (levels[0]=='y') return 0;
    levelno= atoi(levels);
    if (levelno > 0) {
      levelno--;
      if (levelno > 0) sprintf(levels,"%d",levelno);
      else unsetenv(AUTHBIND_LEVELS_VAR);
      return 0;
    }
    unsetenv(AUTHBIND_LEVELS_VAR);
  }
  removepreload();
  return 0;
}

int bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
  pid_t child, rchild;
  char portarg[5], addrarg[9];
  int status;

  if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in) ||
      !geteuid() || ((struct sockaddr_in*)addr)->sin_port == 0 ||
      ntohs(((struct sockaddr_in*)addr)->sin_port) >= IPPORT_RESERVED/2)
    return old_bind(fd,addr,addrlen);

  sprintf(addrarg,"%08lx",
	  ((unsigned long)(((struct sockaddr_in*)addr)->sin_addr.s_addr))&0x0ffffffffUL);
  sprintf(portarg,"%04x",
	  ((unsigned int)(((struct sockaddr_in*)addr)->sin_port))&0x0ffff);

  child= fork(); if (child==-1) return -1;

  if (!child) {
    if (dup2(fd,0)) exiterrno(errno);
    removepreload();
    execl(HELPER,HELPER,addrarg,portarg,(char*)0);
    status= errno;
    STDERRSTR_CONST("libauthbind: possible installation problem - "
		    "could not invoke " HELPER " (");
    STDERRSTR_STRING(rcsid);
    STDERRSTR_CONST(")\n");
    exiterrno(status);
  }

  rchild= waitpid(child,&status,0);
  if (rchild==-1) return -1;
  if (rchild!=child) { errno= ECHILD; return -1; }

  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status)) { errno= WEXITSTATUS(status); return -1; }
    return 0;
  } else {
    errno= ENOSYS;
    return -1;
  }
}
