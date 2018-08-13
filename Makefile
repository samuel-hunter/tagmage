CFLAGS = -Wpedantic -g -O0 `pkg-config --cflags sqlite3`
LDFLAGS = `pkg-config --libs sqlite3`
CC = clang

SRC = util.c database.c
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

tagmage: $(OBJ) tagmage.o
	$(CC) $(LDFLAGS) -o $@ $^ -rdynamic

test: $(OBJ) test.o
	$(CC) $(LDFLAGS) -o $@ $^
	./test

clean:
	rm -f tagmage test $(OBJ) tagmage.o test.o

.PHONY: all options clean test
