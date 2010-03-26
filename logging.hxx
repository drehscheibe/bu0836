#include <iosfwd>

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)": "

#define ALWAYS -1
#define ALERT 0
#define WARN 1
#define INFO 2
#define BULK 3



class color {
public:
	color(const char *c = "") : _color(c) {}
	inline const char *str() const { return _color; }
	static bool _isatty;
private:
	const char *_color;
};



int get_log_level();
void set_log_level(int);
std::ostream &log(int log_level);
std::ostream &operator<<(std::ostream &, const color &);
