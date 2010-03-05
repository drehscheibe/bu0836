/* get_option() return values (if not index of option in struct command_line_option array) */
#define OPTIONS_DONE -1
#define OPTIONS_TERMINATOR -2
#define OPTIONS_ARGUMENT -3
#define OPTIONS_EXCESS_ARGUMENT -4
#define OPTIONS_MISSING_ARGUMENT -5
#define OPTIONS_UNKNOWN_OPTION -6


struct command_line_option {
	const char *long_opt;
	const char *short_opt;
	int has_arg;
};


struct option_parser_data {
	// static data (only stored, but remain unchanged)
	int argc;
	const char **argv;
	const struct command_line_option *options;

	// public data
	const char *option;
	const char *argument;

	// internal data (not meant for public consumption)
	const char *aggregate;
	int index;
	char buf[3];
};


void init_option_parser(struct option_parser_data *data, int argc, const char *argv[],
		const struct command_line_option *options);

int get_option(struct option_parser_data *data);

