//#ifdef __cplusplus
//extern "C" {
//#endif

#define OPTIONS_CONTINUE 0
#define OPTIONS_ABORT 1

#define OPTIONS_ARGUMENT -1
#define OPTIONS_UNKNOWN -2
#define OPTIONS_TERMINATOR -3
#define OPTIONS_MISSING_ARGUMENT -4
#define OPTIONS_EXCESS_ARGUMENT -5


struct Options {
	const char *long_opt;
	const char *short_opt;
	int has_arg;
};

int parse_options(int argc, const char *argv[], struct Options *options, int (*handler)(int index, const char *arg));


//#ifdef __cplusplus
//}
//#endif
