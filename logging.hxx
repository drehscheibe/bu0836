#ifndef _LOGGING_HXX_
#define _LOGGING_HXX_

#include <iosfwd>

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)": "

#define NORM color()
#define BLACK color("30")
#define RED color("31")
#define GREEN color("32")
#define YELLOW color("33")
#define BLUE color("34")
#define MAGENTA color("35")
#define CYAN color("36")
#define WHITE color("37")

#define BBLACK color("1;30")
#define BRED color("1;31")
#define BGREEN color("1;32")
#define BYELLOW color("1;33")
#define BBLUE color("1;34")
#define BMAGENTA color("1;35")
#define BCYAN color("1;36")
#define BWHITE color("1;37")



enum {
	BULK,
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



std::ostream &operator<<(std::ostream &, const color &);
int get_log_level();
void set_log_level(int);
std::ostream &log(int log_level);

#endif
