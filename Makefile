include config.mk


SRC = util.c database.c tagmage.c
OBJ = $(SRC:.c=.o)
HEADERS = util.h database.h

all: options tagmage

options:
	@echo tagmage build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC = $(CC)"
	@echo

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $<

tagmage: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ -rdynamic

install: tagmage
	install -m 755 -d $(MANDIR)/man1 $(PREFIXDIR)/bin

	install -m 644 tagmage.1 tad.1 $(MANDIR)/man1/
	install -m 755 tagmage tad $(PREFIXDIR)/bin/

clean:
	rm -f tagmage test $(OBJ) tagmage.o test.o

.PHONY: all options install clean
