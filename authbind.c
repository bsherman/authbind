/*
 *  authbind.c - main invoker program
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static const char *rcsid="$Id$";

#ifndef LIBAUTHBIND
# define "/usr/local/lib/authbind/libauthbind.so." MAJOR_VER
#endif

#define PRELOAD_VAR "LD_PRELOAD"
#define AUTHBINDLIB_VAR "AUTHBIND_LIB"

int main(int argc, char *const *argv) {
  const char *expreload, *authbindlib, *preload;
  char *newpreload;

  if (argc<2 || argv[1][0]=='-') {
    fprintf(stderr,"authbind: usage: authbind program arg arg ...\n %s\n",rcsid);
    exit(-1);
  }

  authbindlib= getenv(AUTHBINDLIB_VAR);
  if (!authbindlib) authbindlib= LIBAUTHBIND;
    
  if ((expreload= getenv(PRELOAD_VAR))) {
    newpreload= malloc(strlen(expreload)+strlen(authbindlib)+2);
    strcpy(newpreload,expreload);
    strcat(newpreload,":");
    strcat(newpreload,authbindlib);
    preload= newpreload;
  } else {
    preload= authbindlib;
  }
  if (setenv(PRELOAD_VAR,preload,1)) { perror("authbind: setenv"); exit(-1); }

  execvp(argv[1],argv+1);
  perror(argv[1]); exit(-1);
}
