PREFIXDIR=$(HOME)/.local
MANDIR=$(PREFIXDIR)/share/man

CFLAGS = -Wpedantic -g -O0 `pkg-config --cflags sqlite3`
LDFLAGS = `pkg-config --libs sqlite3`
CC = clang
