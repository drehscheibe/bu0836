DEBUG=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_CONCEPT_CHECKS -O0 -g
FLAGS=-mtune=native -pipe -O3 -Wall
FLAGS:=${FLAGS} ${DEBUG} # FIXME DEBUG BUILD

LIBUSB_CFLAGS=`libusb-config --cflags`
LIBUSB_LIBS=`libusb-config --libs`

all: bu0836 makefile
	@echo DEBUG BUILD # FIXME
	./bu0836

check: bu0836
	cppcheck -f --enable=all .

vg valgrind: bu0836
	valgrind --tool=memcheck --leak-check=full ./bu0836 -vvvvv --list --device=00 --monitor
	@#valgrind --tool=exp-ptrcheck ./bu0836 -vvvvv --list --device=00 --monitor

bu0836: logging.o options.o hid_parser.o bu0836.o main.o makefile
	g++ -g -o bu0836 logging.o options.o bu0836.o hid_parser.o main.o ${LIBUSB_LIBS}

main.o: bu0836.hxx logging.hxx options.h main.cxx makefile
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c main.cxx

bu0836.o: bu0836.cxx bu0836.hxx hid_parser.hxx logging.hxx makefile
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c bu0836.cxx

hid_parser.o: hid_parser.cxx hid_parser.hxx logging.hxx makefile
	g++ ${FLAGS} -c hid_parser.cxx

logging.o: logging.cxx logging.hxx makefile
	g++ ${FLAGS} -c logging.cxx

options.o: options.c options.h makefile
	g++ ${FLAGS} -c options.c

clean:
	rm -rf *.o bu0836 core.bu0836.* cmake_install.cmake CMakeFiles CMakeCache.txt

help:
	@echo "targets:"
	@echo "    all"
	@echo "    check            (requires cppcheck)"
	@echo "    vg, valgrind     (requires valgrind)"
	@echo "    clean"
