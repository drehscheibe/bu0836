#include <cstring>     // strcmp
#include <iomanip>
#include <iostream>
#include <stdlib.h>    // getenv, STDOUT_FILENO
#include <sstream>
#include <unistd.h>    // isatty

#include "logging.hxx"

using namespace std;



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

class nullbuf : public streambuf { } nb;
ostream cnull(&nb);

bool cout_color = has_color(STDOUT_FILENO);
bool cerr_color = has_color(STDERR_FILENO);
int log_level = ALERT;

}



ostream &operator<<(ostream &os, const color &c) {
	if ((os == cerr && cerr_color) || (os == cout && cout_color))
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



ostream &log(int priority = ALWAYS)
{
	return priority >= log_level ? cerr : cnull;
}



string bytes(const unsigned char *p, unsigned int num, size_t width)
{
	ostringstream x;
	x << hex << setfill('0');
	while (num--)
		x << setw(2) << int(*p++) << ' ';
	string s = x.str();
	if (width > s.length())
		s.resize(width, ' ');
	return s;
}
