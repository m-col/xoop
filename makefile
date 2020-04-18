OUT        = xinf
SRC 	   = xinf.c
CFLAGS 	  += -Wall -Wextra -pedantic -lxcb
PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin

all: $(OUT)

clean:
	$(RM) $(OUT) $(OBJ)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf xinf "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/xinf


.PHONY: all clean install uninstall
