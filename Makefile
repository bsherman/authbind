#
OPTIMISE=	-O2
CFLAGS=		-g $(OPTIMISE) \
		-Wall -Wwrite-strings -Wpointer-arith -Wimplicit \
		-Wnested-externs -Wmissing-prototypes -Wstrict-prototypes

TARGETS=		authbind helper libauthbind.so.0.1

all:			$(TARGETS)

libauthbind.o:		libauthbind.c
		gcc -D_REENTRANT -g $(CFLAGS) -c -o libauthbind.o -fPIC libauthbind.c

libauthbind.so.0.1:	libauthbind.o
		gcc -g -shared -Wl,-soname,libauthbind.so.0.1 -o libauthbind.so.0.1 libauthbind.o -ldl -lc

clean distclean: 
		rm -f $(TARGETS) *.o *~ ./#*# *.bak debian/*~
		rm -f core libauthbind.so*
