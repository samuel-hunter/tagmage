VERSION := 0.1.1

PREFIX ?= /usr/local
MANPREFIX := $(PREFIX)/share/man

CFLAGS := -Wpedantic -Werror -std=c99 -O2 `pkg-config --cflags sqlite3`
LDFLAGS := `pkg-config --libs sqlite3` -lbsd


SRCDIR := src
BUILDDIR := build

SRC := $(SRCDIR)/util.c $(SRCDIR)/database.c $(SRCDIR)/tagmage.c
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))
HEADERS := $(SRCDIR)/util.h $(SRCDIR)/database.h

DISTFILES := $(SRCDIR) tad tagmage.1 tad.1 Makefile LICENSE README.md


all: options tagmage

options:
	@echo tagmage build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC = $(CC)"
	@echo "PREFIX = $(PREFIX)"
	@echo "MANPREFIX = $(MANPREFIX)"
	@echo

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

tagmage: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ -rdynamic

install: tagmage
	install -m 755 -d $(MANPREFIX)/man1 $(PREFIX)/bin
	install -m 755 tagmage tad $(PREFIX)/bin/

	sed "s/@@VERSION@@/$(VERSION)/g" < tagmage.1 > $(MANPREFIX)/man1/tagmage.1
	sed "s/@@VERSION@@/$(VERSION)/g" < tad.1 > $(MANPREFIX)/man1/tad.1
	chmod 644 $(MANPREFIX)/man1/tagmage.1 $(MANPREFIX)/man1/tad.1


uninstall:
	rm -f $(MANPREFIX)/man1/tagmage.1 $(MANPREFIX)/man1/tad.1
	rm -f $(PREFIX)/bin/tagmage $(PREFIX)/bin/tad

dist:
	mkdir -p tagmage-$(VERSION)
	cp -r $(DISTFILES) tagmage-$(VERSION)
	tar -cf tagmage-$(VERSION).tar tagmage-$(VERSION)
	gzip tagmage-$(VERSION).tar
	rm -r tagmage-$(VERSION)

clean:
	rm -rf $(BUILDDIR) tagmage tagmage-*.tar.gz

.PHONY: all options install uninstall dist clean
