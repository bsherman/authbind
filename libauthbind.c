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

#ifndef HELPER
# define HELPER "/usr/local/lib/authbind/helper"
#endif

#define AUTHBIND_NESTED_VAR "AUTHBIND_NESTED"

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

int bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
  pid_t child, rchild;
  char portarg[5], addrarg[9];
  int status;
  
  if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in) ||
      ntohs(((struct sockaddr_in*)addr)->sin_port) >= IPPORT_RESERVED/2 || !geteuid())
    return old_bind(fd,addr,addrlen);

  if (getenv(AUTHBIND_NESTED_VAR)) {
    STDERRSTR_CONST("libauthbind: possible installation problem - "
		    "nested invocation, perhaps helper is not setuid\n ");
    STDERRSTR_STRING(rcsid);
    STDERRSTR_CONST("\n");
    return old_bind(fd,addr,addrlen);
  }

  sprintf(addrarg,"%08lx",
	  ((unsigned long)(((struct sockaddr_in*)addr)->sin_addr.s_addr))&0x0ffffffffUL);
  sprintf(portarg,"%04x",
	  ((unsigned int)(((struct sockaddr_in*)addr)->sin_port))&0x0ffff);

  child= fork(); if (child==-1) return -1;

  if (!child) {
    if (dup2(fd,0)) exiterrno(errno);
    if (setenv(AUTHBIND_NESTED_VAR,"1",1)) exiterrno(errno);
    execl(HELPER,HELPER,addrarg,portarg,(char*)0);
    status= errno;
    STDERRSTR_CONST("libauthbind: possible installation problem - "
		    "could not invoke " HELPER "\n");
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
