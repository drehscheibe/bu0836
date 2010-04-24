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
#ifndef _LOGGING_HXX_
#define _LOGGING_HXX_

#include <iosfwd>
#include <string>

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)": "



namespace logging {

enum {
	BULK = 1,
	DEBUG,
	INFO,
	WARN,
	ALERT,
	ALWAYS = 1000,
};



class color {
public:
	color(const char *c = "") : _color(c) {}

private:
	friend std::ostream &operator<<(std::ostream &, const color &);
	const char *_color;
};



extern color reset;
extern color bold;
extern color underline;
extern color blink;
extern color reverse;

extern color black;
extern color red;
extern color green;
extern color brown;
extern color blue;
extern color magenta;
extern color cyan;
extern color white;

extern color bg_black;
extern color bg_red;
extern color bg_green;
extern color bg_brown;
extern color bg_blue;
extern color bg_magenta;
extern color bg_cyan;
extern color bg_white;



std::ostream &operator<<(std::ostream &, const color &);
int get_log_level();
void set_log_level(int);
std::ostream &log(int log_level);
std::string bytes(const unsigned char *p, unsigned int num, size_t width = 0);

} // namespace logging

#endif
