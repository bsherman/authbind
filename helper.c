/*
 * setuid.  Invoked with socket on stdin.
 *  Usage:  helper <addr> <port>
 * both are hex strings, padded to the right length.
 * they are pairs of hex digits for each byte (network byte order)
 *
 * If /etc/authbind cannot be chdir'd into, is an error.
 *
 * First, check /etc/authbind/byport/<port> with access(2,X_OK).
 *  If OK, then authorised.
 *  If ENOENT then keep looking.
 *  Otherwise, not authorised, errno=whatever
 *
 * Then check /etc/authbind/byboth/<addr>:<port> likewise.
 *
 * Then try to read /etc/authbind/byuid/<uid> (with superuser privs!)
 *  If ENOENT, then not authorised, errno=EPERM
 *  If cannot open, then not authorised, errno=whatever
 *  If it contains a line of the form
 *     <addr>/<length>:<port-min>,<port-max>
 *    then authorised, otherwise not authorised, errno=ENOENT
 *  If read error then is an error
 *
 * In each case,
 *  <addr> is dotted quad
 *  <port> is decimal in host order
 *  <length> is prefix length (so 0.0.0.0/32 matches any)
 *  <uid> is decimal unsigned
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONFIGDIR "/etc/authbind"

static void exiterrno(int e) {
  exit(e>0 && e<128 ? e : ENOSYS);
}

static void perrorfail(const char *m) {
  int e;
  e= errno;
  fprintf(stderr,"libauthbind's helper: %s: %s\n",m,strerror(e));
  exiterrno(e);
}

static void badusage(void) {
  fputs("libauthbind's helper: bad usage\n",stderr);
  exit(ENOSYS);
}

static struct sockaddr_in saddr;

static void authorised(void) {
  if (bind(0,&saddr,sizeof(saddr))) exiterrno(errno);
  else _exit(0);
}

int main(int argc, const char *const *argv) {
  uid_t uid;
  char fnbuf[100];
  char *ep;
  const char *np;
  unsigned long addr, port, haddr, thaddr, thmask;
  unsigned int hport, a1,a2,a3,a4, alen,pmin,pmax;
  int nchar;
  FILE *file;

  if (argc != 3) badusage(); 
  addr= strtoul(argv[1],&ep,16); if (*ep || addr&~0x0ffffffffUL) badusage();
  port= strtoul(argv[2],&ep,16); if (*ep || port&~0x0ffffUL) badusage();

  if (chdir(CONFIGDIR)) perrorfail("chdir " CONFIGDIR);

  fnbuf[sizeof(fnbuf)-1]= 0;
  memset(&saddr,0,sizeof(saddr));
  saddr.sin_family= AF_INET;
  saddr.sin_port= port;
  saddr.sin_addr.s_addr= addr;
  hport= htons(port);

  snprintf(fnbuf,sizeof(fnbuf)-1,"byport/%u",hport);
  if (!access(fnbuf,X_OK)) authorised();
  if (errno != ENOENT) exiterrno(errno);

  np= inet_ntoa(saddr.sin_addr); assert(np);
  snprintf(fnbuf,sizeof(fnbuf)-1,"byaddr/%s:%u",np,hport);
  if (!access(fnbuf,X_OK)) authorised();
  if (errno != ENOENT) exiterrno(errno);

  uid= getuid(); if (uid==(uid_t)-1) perrorfail("getuid");
  snprintf(fnbuf,sizeof(fnbuf)-1,"byuid/%lu",(unsigned long)uid);

  file= fopen(fnbuf,"r");
  if (!file) exiterrno(errno==ENOENT ? EPERM : errno);

  haddr= ntohl(addr);

  while (fgets(fnbuf,sizeof(fnbuf)-1,file)) {
    nchar= -1;
    sscanf(fnbuf," %u.%u.%u.%u/%u:%u,%u %n",
	   &a1,&a2,&a3,&a4,&alen,&pmin,&pmax,&nchar);
    if (nchar != strlen(fnbuf) ||
	alen>32 || pmin&~0x0ffff || pmax&~0x0ffff ||
	a1&~0x0ff || a2&~0xff || a3&~0x0ff || a4&~0x0ff)
      continue;
    
    if (hport<pmin || hport>pmax) continue;

    thaddr= (a1<<24)|(a2<<16)|(a3<<8)|(a4);
    thmask= 0x0ffffffffUL<<(32-alen);
    if ((haddr&thmask) != thaddr) continue;
    authorised();
  }
  if (ferror(file)) perrorfail("read per-uid file");
  _exit(ENOENT);
}
