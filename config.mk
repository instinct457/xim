# xim version
VERSION = 0.1

# paths
PREFIX = /usr/local

# flags
DBUS_CFLAGS = $(shell pkg-config --cflags dbus-1)
DBUS_LIBS   = $(shell pkg-config --libs dbus-1)

CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L ${DBUS_CFLAGS}
CFLAGS   = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
LDFLAGS  = -lX11 -lXext ${DBUS_LIBS}

# compiler and linker
CC = cc
