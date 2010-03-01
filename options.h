/* handler return values */
#define OPTIONS_CONTINUE 0
#define OPTIONS_ABORT 1

/* handler signals */
#define OPTIONS_TERMINATOR -1
#define OPTIONS_ARGUMENT -2
#define OPTIONS_EXCESS_ARGUMENT -3
#define OPTIONS_MISSING_ARGUMENT -4
#define OPTIONS_UNKNOWN_OPTION -5


struct Options {
	const char *long_opt;
	const char *short_opt;
	int has_arg;
};

int parse_options(int argc, const char *argv[], struct Options *options, int (*handler)(int index, const char *arg));

