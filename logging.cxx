#include <cstring>     // strcmp
#include <iostream>
#include <stdlib.h>    // getenv, STDOUT_FILENO
#include <unistd.h>    // isatty

#include "logging.hxx"



namespace {
	bool has_color(int fd)
	{
		if (isatty(fd)) {
			const char *term = getenv("TERM");
			if (term && strcmp(term, "dumb"))
				return true;
		}
		return false;
	}

	class nullbuf : public std::streambuf { } nb;
	std::ostream cnull(&nb);

	bool cout_color = has_color(STDOUT_FILENO);
	bool cerr_color = has_color(STDERR_FILENO);
	int log_level = ALERT;
}



std::ostream &operator<<(std::ostream &os, const color &c) {
	if ((os == std::cerr && cerr_color) || (os == std::cout && cout_color))
		return os << "\033[" << c._color << 'm';
	return os;
}



int get_log_level()
{
	return log_level;
}



void set_log_level(int level)
{
	log_level = level;
}



std::ostream &log(int priority = ALWAYS)
{
	return priority >= log_level ? std::cerr : cnull;
}
