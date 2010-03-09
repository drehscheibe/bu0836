#include <iosfwd>

#define ALWAYS -1
#define ALERT 0
#define WARN 1
#define INFO 2
#define BULK 3

int get_log_level();
void set_log_level(int);
std::ostream& log(int log_level, bool condition = true);
