OUT        = xoop
SRC 	   = xoop.c
CFLAGS 	  += -Wall -Wextra -pedantic -lxcb -lxcb-shape
PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin

all: $(OUT)

debug: CFLAGS += -g -DDEBUG
debug: all

clean:
	$(RM) $(OUT) $(OBJ)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf "$(OUT)" "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(OUT)"


.PHONY: all debug clean install uninstall
