PREFIX ?= /usr/local
MANDIR := $(PREFIX)/share/man

CFLAGS := -Wpedantic -std=c99 -g -O0 `pkg-config --cflags sqlite3`
LDFLAGS := `pkg-config --libs sqlite3`
CC := clang
