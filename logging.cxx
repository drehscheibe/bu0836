// logging utilities
//
// Copyright (C) 2010  Melchior FRANZ  <melchior.franz@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#include <cstring>     // strcmp
#include <iomanip>
#include <iostream>
#include <stdlib.h>    // getenv, STDOUT_FILENO
#include <sstream>
#include <unistd.h>    // isatty

#include "logging.hxx"

using namespace std;



namespace logging {

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

} // namespace



color reset("0");
color bold("1");
color underline("4");
color blink("5");
color reverse("7");

color black("30");
color red("31");
color green("32");
color brown("33");
color blue("34");
color magenta("35");
color cyan("36");
color white("37");

color bg_black("40");
color bg_red("41");
color bg_green("42");
color bg_brown("43");
color bg_blue("44");
color bg_magenta("45");
color bg_cyan("46");
color bg_white("47");



ostream &operator<<(ostream &os, const color &c) {
	if ((os == cerr && cerr_color) || (os == cout && cout_color))
		return os << "\033[" << c._color << 'm';
	return os;
}



string operator+(const string &s, int i)
{
	ostringstream x;
	x << i;
	return s + x.str();
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

} // namespace logging
