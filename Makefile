#
#  Makefile
#
#  Copyright (c) 2021 by Daniel Kelley
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
LDLIBS := -ligraph -lplot -lgeos_c -lwkt -lm

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

WKTDEL_SRC := wktdel.c
WKTDEL_OBJ := $(WKTDEL_SRC:%.c=%.o)
WKTDEL_DEP := $(WKTDEL_SRC:%.c=%.d)
OBJ += $(WKTDEL_OBJ)
DEP += $(WKTDEL_DEP)

WKTVOR_SRC := wktvor.c
WKTVOR_OBJ := $(WKTVOR_SRC:%.c=%.o)
WKTVOR_DEP := $(WKTVOR_SRC:%.c=%.d)
OBJ += $(WKTVOR_OBJ)
DEP += $(WKTVOR_DEP)

WKTHULL_SRC := wkthull.c
WKTHULL_OBJ := $(WKTHULL_SRC:%.c=%.o)
WKTHULL_DEP := $(WKTHULL_SRC:%.c=%.d)
OBJ += $(WKTHULL_OBJ)
DEP += $(WKTHULL_DEP)

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

PROG := wktplot wktrand wktdel wktvor wkthull

VG ?= valgrind --leak-check=full

.PHONY: all install uninstall clean test check

all: $(PROG) $(LIBRARY) $(SHLIBRARY)

wktplot: $(WKTPLOT_SRC) $(SHLIBRARY)

wktrand: $(WKTRAND_SRC) $(SHLIBRARY)

wktdel: $(WKTDEL_SRC) $(SHLIBRARY)

wktvor: $(WKTVOR_SRC) $(SHLIBRARY)

wkthull: $(WKTHULL_SRC) $(SHLIBRARY)

$(LIBRARY): $(WKTLIB_OBJ)
	$(AR) cr $@ $^

$(SHLIBRARY): $(SHLIBRARY_VER)
	ln -sf $< $@

$(SHLIBRARY_VER): $(WKTLIB_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) $(WKTLIB_LDLIBS) $(WKTLIB_OBJ)

install: $(PROG) $(SHLIBRARY) $(LIBRARY)
	install -p -m 644 wkt.h $(PREFIX)/include
	install -p -m 755 $(PROG) $(PREFIX)/bin
	install -p -m 644 $(SHLIBRARY_VER) $(LIBRARY) $(PREFIX)/lib
	ln -sf -r $(PREFIX)/lib/$(SHLIBRARY_VER) $(PREFIX)/lib/$(SHLIBRARY)

uninstall:
	-rm -f $(PREFIX)/bin/wktplot
	-rm -f $(PREFIX)/include/wkt.h
	-rm -f $(PREFIX)/lib/$(SHLIBRARY)
	-rm -f $(PREFIX)/lib/$(SHLIBRARY_VER)
	-rm -f $(PREFIX)/lib/$(LIBRARY)

rr.wkt: wktrand
	LD_LIBRARY_PATH=. ./wktrand -u -q 0.5 -n 8 -x 10 -y 10 $@

del.wkt: rr.wkt wktdel
	LD_LIBRARY_PATH=. ./wktdel $< $@

vor.wkt: rr.wkt wktvor
	LD_LIBRARY_PATH=. ./wktvor $< $@

hull.wkt: rr.wkt wkthull
	LD_LIBRARY_PATH=. ./wkthull $< $@

ring.wkt: rr.wkt wkthull
	LD_LIBRARY_PATH=. ./wkthull -r $< $@

test: $(PROG) rr.wkt del.wkt vor.wkt hull.wkt ring.wkt
	LD_LIBRARY_PATH=. ./wktplot -TX -p5,0.9 rr.wkt
	LD_LIBRARY_PATH=. ./wktplot -TX del.wkt
	LD_LIBRARY_PATH=. ./wktplot -TX vor.wkt
	LD_LIBRARY_PATH=. ./wktplot -TX hull.wkt
	LD_LIBRARY_PATH=. ./wktplot -TX ring.wkt

check: $(PROG) rr.wkt del.wkt vor.wkt hull.wkt ring.wkt
	LD_LIBRARY_PATH=. ./wktplot -Tsvg -p5,0.9 rr.wkt > rr.svg
	LD_LIBRARY_PATH=. ./wktplot -Tsvg del.wkt > del.svg
	LD_LIBRARY_PATH=. ./wktplot -Tsvg vor.wkt > vor.svg
	LD_LIBRARY_PATH=. ./wktplot -Tsvg hull.wkt > hull.svg

#
# libplot is a bit leaky, but svg and X plotter is leakier than ps
#
valgrind-test: $(PROG) rr.wkt del.wkt vor.wkt hull.wkt ring.wkt
	LD_LIBRARY_PATH=. $(VG) ./wktrand -u -q 0.5 -n 8 -x 10 -y 10 rr.wkt
	LD_LIBRARY_PATH=. $(VG) ./wktdel rr.wkt del.wkt
	LD_LIBRARY_PATH=. $(VG) ./wktvor rr.wkt vor.wkt
	LD_LIBRARY_PATH=. $(VG) ./wkthull rr.wkt hull.wkt
	LD_LIBRARY_PATH=. $(VG) ./wkthull -r rr.wkt ring.wkt
	LD_LIBRARY_PATH=. $(VG) ./wktplot -Tps del.wkt > /dev/null
	LD_LIBRARY_PATH=. $(VG) ./wktplot -Tps -p5,0.9 rr.wkt > /dev/null

clean:
	-rm -f $(PROG) $(SHLIBRARY) $(SHLIBRARY_VER) $(LIBRARY) \
		$(OBJ) $(DEP) *.wkt *.svg *.ps

-include $(DEP)
