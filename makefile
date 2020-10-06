OUT        = xoop
SRC 	   = xoop.c
CFLAGS 	  += -Wall -Wextra -pedantic -lxcb -lxcb-randr -lxcb-xinput -lxcb-xfixes
PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin

all: clean $(OUT)

clean:
	$(RM) $(OUT) $(OBJ)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf "$(OUT)" "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(OUT)"


.PHONY: all debug clean install uninstall
