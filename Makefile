# xim - X11 idle manager
include config.mk

SRC = xim.c
OBJ = ${SRC:.c=.o}

all: xim

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

xim: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f xim ${OBJ} xim-${VERSION}.tar.gz

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f xim ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/xim

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xim

.PHONY: all clean install uninstall
