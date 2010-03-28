#ifndef _LOGGING_HXX_
#define _LOGGING_HXX_

#include <iosfwd>

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)": "


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
