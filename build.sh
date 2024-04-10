#!/bin/sh

if [ -z "$1" ]; then
    cat >&2 <<EOF
Usage: $0 foo
  compiles  map_foo.c
  into      xoop_foo

\$CC overrides the choice of C compiler.
EOF
    exit 1
fi

set -xe

${CC:-cc} -Wall -Wextra -pedantic \
  -o xoop_$1 xoop.c map_$1.c \
  -lxcb -lxcb-randr -lxcb-xinput -lxcb-xfixes
