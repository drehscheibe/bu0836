#include <iostream>
#include "logging.h"


namespace {
	int log_level = 1;

	class nullbuf : public std::streambuf { } nb;
	std::ostream cnull(&nb);
}


int get_log_level()
{
	return log_level;
}


void set_log_level(int level)
{
	log_level = level;
}


std::ostream& log(int level = ALWAYS, bool condition)
{
	return condition && level < log_level ? std::cerr : cnull;
}
