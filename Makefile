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
	mkdir -p $(MANDIR)/man1
	install -m 644 tagmage.1 $(MANDIR)/man1/tagmage.1
	mkdir -p $(PREFIXDIR)/bin
	install -m 755 tagmage $(PREFIXDIR)/bin/tagmage
	install -m 755 tad $(PREFIXDIR)/bin/tad

clean:
	rm -f tagmage test $(OBJ) tagmage.o test.o

.PHONY: all options install clean
