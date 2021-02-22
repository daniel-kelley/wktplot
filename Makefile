#
#  Makefile
#
#  Copyright (c) 2012 by Daniel Kelley
#

NAME := wktplot

DEBUG ?= -g

PREFIX ?= /usr/local

# address thread undefined etc.
ifneq ($(SANITIZE),)
DEBUG += -fsanitize=$(SANITIZE)
endif

INC :=
CPPFLAGS := $(INC) -MP -MMD

WARN := -Wall
WARN += -Wextra
WARN += -Wdeclaration-after-statement
WARN += -Werror
CFLAGS := $(WARN) $(DEBUG)

LDFLAGS := $(DEBUG)
LDLIBS := -lplot -lgeos_c

SRC := wktplot.c

OBJ := $(SRC:%.c=%.o)
DEP := $(SRC:%.c=%.d)

PROG := $(NAME)

.PHONY: all install uninstall clean

all: $(PROG)

install: $(PROG)
	install -p -m 755 $< $(PREFIX)/bin

uninstall:
	-rm -f $(PREFIX)/bin/$(PROG)

clean:
	-rm -f $(PROG) $(OBJ) $(DEP)

-include $(DEP)
