#include <iostream>
#include <string>
#include <unistd.h>

#include "logging.h"


namespace {
	int log_level = 1;

	class nullbuf : public std::streambuf { } nb;
	std::ostream cnull(&nb);
}


bool color::_isatty = isatty(2);



std::ostream &operator<<(std::ostream &os, const color &c) {
	return color::_isatty ? os << "\033[" << c.str() << 'm' : os;
}



int get_log_level()
{
	return log_level;
}



void set_log_level(int level)
{
	log_level = level;
}


std::ostream &log(int level = ALWAYS, bool condition)
{
	return condition && level < log_level ? std::cerr : cnull;
}
