include config.mk

SRCDIR = src
BUILDDIR = build


SRC = $(SRCDIR)/util.c $(SRCDIR)/database.c $(SRCDIR)/tagmage.c
OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))
HEADERS = $(SRCDIR)/util.h $(SRCDIR)/database.h

all: options tagmage

options:
	@echo tagmage build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC = $(CC)"
	@echo

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

tagmage: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ -rdynamic

install: tagmage
	install -m 755 -d $(MANDIR)/man1 $(PREFIXDIR)/bin

	install -m 644 tagmage.1 tad.1 $(MANDIR)/man1/
	install -m 755 tagmage tad $(PREFIXDIR)/bin/

clean:
	rm -rf $(BUILDDIR) tagmage

.PHONY: all options install clean
