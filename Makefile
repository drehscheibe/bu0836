DEBUG=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_CONCEPT_CHECKS -O0 -g
FLAGS=-mtune=native -pipe -O3 -Wall
FLAGS:=${FLAGS} ${DEBUG} # FIXME DEBUG BUILD

LIBUSB_CFLAGS=`libusb-config --cflags`
LIBUSB_LIBS=`libusb-config --libs`

all: bu0836a Makefile
	@echo DEBUG BUILD # FIXME
	./bu0836a

check: bu0836a
	cppcheck -f --enable=all .

vg valgrind: bu0836a
	valgrind --tool=memcheck --leak-check=full ./bu0836a -vvvvv --list --device=00 --monitor
	@#valgrind --tool=exp-ptrcheck ./bu0836a -vvvvv --list --device=00 --monitor

bu0836a: logging.o options.o hid_parser.o bu0836a.o main.o Makefile
	g++ -g -o bu0836a logging.o options.o bu0836a.o hid_parser.o main.o ${LIBUSB_LIBS}

main.o: bu0836a.hxx logging.hxx options.h main.cxx Makefile
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c main.cxx

bu0836a.o: bu0836a.cxx bu0836a.hxx hid_parser.hxx logging.hxx Makefile
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c bu0836a.cxx

hid_parser.o: hid_parser.cxx hid_parser.hxx logging.hxx Makefile
	g++ ${FLAGS} -c hid_parser.cxx

logging.o: logging.cxx logging.hxx Makefile
	g++ ${FLAGS} -c logging.cxx

options.o: options.c options.h Makefile
	g++ ${FLAGS} -c options.c

clean:
	rm -f *.o bu0836a core.bu0836a.*

