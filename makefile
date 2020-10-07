OUT        = xoop
SRC 	   = xoop.c
CFLAGS 	  += -Wall -Wextra -pedantic -lxcb -lxcb-randr -lxcb-xinput -lxcb-xfixes
PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/man/man1

all: clean $(OUT)

clean:
	$(RM) $(OUT) $(OBJ)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf "$(OUT)" "$(DESTDIR)$(BINPREFIX)"
	mkdir -p "${DESTDIR}${MANPREFIX}"
	cp -pf "${OUT}.1" "${DESTDIR}${MANPREFIX}"


uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(OUT)"
	rm -f "$(DESTDIR)$(MANPREFIX)/$(OUT).1"


.PHONY: all debug clean install uninstall
