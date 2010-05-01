PREFIX ?= /usr/local
MANDIR ?= $(PREFIX)/share/man
INSTALL ?= install

CXXFLAGS ?= -mtune=native -pipe -O3 -Wall
CFLAGS ?= -mtune=native -pipe -O3 -Wall
LDFLAGS ?= -g

DEBUG = -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_CONCEPT_CHECKS -O0 -g
SHA = $(shell git log master --pretty=format:%h -1)
TAG = $(shell git tag -l '[0-9].*'|tail -1|tr -d '\n')
MOD = $(shell git diff --shortstat|wc -l)

LIBUSB_CFLAGS = $(shell pkg-config libusb-1.0 --cflags)
LIBUSB_LIBS = $(shell pkg-config libusb-1.0 --libs)

ifeq ($(MAKECMDGOALS),vg)
VALGRIND = -DVALGRIND
endif

ifeq ($(MAKECMDGOALS),massif)
VALGRIND = -DVALGRIND
endif

ifeq ($(MAKECMDGOALS),static)
CXXFLAGS += -m32
CFLAGS += -m32
endif

ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS += $(DEBUG) # FIXME DEBUG BUILD
CFLAGS += -g
endif

all: bu0836 makefile

debug: bu0836 makefile
	@echo DEBUG BUILD # FIXME

bu0836: logging.o options.o hid.o bu0836.o main.o makefile
	g++ $(LDFLAGS) -o bu0836 logging.o options.o bu0836.o hid.o main.o -lm $(LIBUSB_LIBS)

main.o: bu0836.hxx logging.hxx options.h main.cxx makefile
	g++ $(CXXFLAGS) -DSHA=$(SHA) -DTAG=$(TAG) -DMOD=$(MOD) -I/usr/include/libusb-1.0 -c main.cxx

bu0836.o: bu0836.cxx bu0836.hxx hid.hxx logging.hxx makefile
	g++ $(CXXFLAGS) $(VALGRIND) -I/usr/include/libusb-1.0 -c bu0836.cxx

hid.o: hid.cxx hid.hxx logging.hxx makefile
	g++ $(CXXFLAGS) -c hid.cxx

logging.o: logging.cxx logging.hxx makefile
	g++ $(CXXFLAGS) -c logging.cxx

options.o: options.c options.h makefile
	g++ $(CFLAGS) -c options.c

static: logging.o options.o hid.o bu0836.o main.o makefile
	g++ -m32 $(LDFLAGS) -o bu0836-static32 logging.o options.o bu0836.o hid.o main.o /usr/lib/libusb-1.0.a -lrt -pthread -lm

check: bu0836
	cppcheck -f -q --enable=all .

vg: bu0836
	valgrind --tool=memcheck --leak-check=full ./bu0836 -vvvvv --list --device=0 --axes=0,2,4-6 -X --monitor --axes=0-7 --invert=0 --zoom=0 --buttons=0-31 --encoder=0
	@#valgrind --tool=exp-ptrcheck ./bu0836 -vvvvv --list --device=00 --monitor

massif: bu0836
	valgrind --tool=massif ./bu0836 -vvvvv --list --device=00 --monitor

pdf:
	@man -lt bu0836.1 >bu0836.ps && ps2pdf bu0836.ps && rm bu0836.ps

install: bu0836 bu0836.1
	$(INSTALL) -m755 bu0836 $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m644 bu0836.1 $(DESTDIR)$(MANDIR)/man1

clean:
	rm -rf *.o bu0836 bu0836-static32 core.bu0836.* cmake_install.cmake install_manifest.txt Makefile CMakeFiles CMakeCache.txt

help:
	@echo "targets:"
	@echo "    all"
	@echo "    check            (requires cppcheck)"
	@echo "    vg               (requires valgrind)"
	@echo "    pdf              make pdf verion of man page"
	@echo "    massif"
	@echo "    static           build 32 bit version with statically linked libusb"
	@echo "    clean"

.PHONY: all debug static check vg massif pdf install clean help
