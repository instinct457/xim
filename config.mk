# xim version
VERSION = 0.1

# paths
PREFIX = /usr/local

# flags (qui inseriamo le librerie X11 e Xext che usavamo da riga di comando)
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS   = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
LDFLAGS  = -lX11 -lXext

# compiler and linker
CC = cc
