/**/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define LD_PRELOAD "LD_PRELOAD"
#define AUTHBINDLIB "AUTHBIND_LIB"
#define LIBAUTHBIND "/usr/lib/authbind/libauthbind.so.0"

int main(int argc, char *const *argv) {
  const char *expreload, *authbindlib, *preload;
  char *newpreload;

  authbindlib= getenv(AUTHBINDLIB);
  if (!authbindlib) {
    if (setenv(AUTHBINDLIB,LIBAUTHBIND,0)) {
      perror("authbind: setenv " AUTHBINDLIB);
      exit(-1);
    }
    authbindlib= LIBAUTHBIND;
  }
    
  if ((expreload= getenv(LD_PRELOAD))) {
    newpreload= malloc(strlen(expreload)+strlen(authbindlib)+2);
    strcpy(newpreload,expreload);
    strcat(newpreload,":");
    strcat(newpreload,authbindlib);
    preload= newpreload;
  } else {
    preload= authbindlib;
  }
  if (setenv(LD_PRELOAD,preload,1)) { perror("authbind: setenv"); exit(-1); }

  execvp(argv[1],argv+1);
  perror(argv[1]); exit(-1);
}
