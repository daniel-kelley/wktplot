#
#  Makefile
#
#  Copyright (c) 2012 by Daniel Kelley
#

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
CFLAGS := $(WARN) $(DEBUG) -fPIC

LDFLAGS := $(DEBUG) -L.
LDLIBS := -lplot -lgeos_c -lwkt

WKTPLOT_SRC := wktplot.c
WKTPLOT_OBJ := $(WKTPLOT_SRC:%.c=%.o)
WKTPLOT_DEP := $(WKTPLOT_SRC:%.c=%.d)
OBJ := $(WKTPLOT_OBJ)
DEP := $(WKTPLOT_DEP)

WKTRAND_SRC := wktrand.c
WKTRAND_OBJ := $(WKTRAND_SRC:%.c=%.o)
WKTRAND_DEP := $(WKTRAND_SRC:%.c=%.d)
OBJ += $(WKTRAND_OBJ)
DEP += $(WKTRAND_DEP)

LIBMAJOR := 0
LIBMINOR := 1

LIBNAME := libwkt
LIBRARY := $(LIBNAME).a
SHLIBRARY := $(LIBNAME).so
SHLIBRARY_VER := $(LIBNAME)-$(LIBMAJOR).$(LIBMINOR).so

WKTLIB_SRC := wkt_open.c
WKTLIB_SRC += wkt_read.c
WKTLIB_SRC += wkt_snag.c
WKTLIB_SRC += wkt_close.c
WKTLIB_SRC += wkt_iterate_coord_seq.c
WKTLIB_SRC += wkt_bounds.c
WKTLIB_SRC += wkt_iterate.c
WKTLIB_SRC += wkt_write.c
WKTLIB_SRC += wkt_stash.c
WKTLIB_LDLIBS := -lgeos_c
WKTLIB_OBJ := $(WKTLIB_SRC:%.c=%.o)
WKTLIB_DEP := $(WKTLIB_SRC:%.c=%.d)
OBJ += $(WKTLIB_OBJ)
DEP += $(WKTLIB_DEP)

PROG := wktplot wktrand

.PHONY: all install uninstall clean

all: $(PROG) $(LIBRARY) $(SHLIBRARY)

wktplot: $(WKTPLOT_SRC) $(SHLIBRARY)

wktrand: $(WKTRAND_SRC) $(SHLIBRARY)

$(LIBRARY): $(WKTLIB_OBJ)
	$(AR) cr $@ $^

$(SHLIBRARY): $(SHLIBRARY_VER)
	ln -sf $< $@

$(SHLIBRARY_VER): $(WKTLIB_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) $(WKTLIB_LDLIBS) $(WKTLIB_OBJ)

install: $(PROG) $(SHLIBRARY) $(LIBRARY)
	install -p -m 755 wktplot $(PREFIX)/bin
	install -p -m 755 $(SHLIBRARY) $(PREFIX)/lib
	install -p -m 755 $(LIBRARY) $(PREFIX)/lib

uninstall:
	-rm -f $(PREFIX)/bin/wktplot

clean:
	-rm -f $(PROG) $(SHLIBRARY) $(SHLIBRARY_VER) $(LIBRARY) $(OBJ) $(DEP)

-include $(DEP)
