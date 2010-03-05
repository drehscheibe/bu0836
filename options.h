/*
Syntax:

	-v --verbose  ... short and long option
	-v -vvv       ... single and aggregate short option (equivalent to -v -v -v -v)
	-d foo        ... short option with argument
	-dfoo         ... same
	-vdfoo        ... interpreted as -v -d foo  (if -v doesn't take argument, and -d does)
	--device=foo  ... long option with argument
	--device foo  ... same
	--device=     ... long option with empty argument
	--device ""   ... same
	--device=""   ... not the same! Interpreted as --device '""'!
	--            ... terminates option parsing; remaining elements are treated as argument
	-             ... argument '-'


Example:

	const struct command_line_option options {
		{ "--help", "-h", 0 },
		{ "--file", -f", 1 },                   // takes argument
		{ "--long", 0, 0 },                     // only long option available
		{ 0, "-s", 0 },                         // only short option available
		OPTIONS_LAST                            // mandatory last entry
	};

	int opt;
	static struct option_parser_data d;
	init_option_parser(&d, argc, argv, options);
	while ((opt = get_option(&d)) != OPTIONS_DONE) {
		switch (opt) {
		case 0: cerr << "HELP" << endl; break;
		case 1: cerr << "LONG" << endl; break;
		case 2: cerr << "SHORT" << endl; break;
		case 3: cerr << "FILE " << d.argument << endl; break;
		case OPTIONS_ARGUMENT: cerr << "non-option argument " << d.argument << endl; break;
		case OPTIONS_TERMINATOR: break;         // ignore "--"
		default: cerr << "bad option" << endl;  // lazy handling of all other error codes
		}
	}

*/


/* get_option() returns a "struct command_line_option" index, or one of the following codes */
#define OPTIONS_DONE -1
#define OPTIONS_TERMINATOR -2
#define OPTIONS_ARGUMENT -3
#define OPTIONS_EXCESS_ARGUMENT -4
#define OPTIONS_MISSING_ARGUMENT -5
#define OPTIONS_UNKNOWN_OPTION -6

#define OPTIONS_LAST { 0, 0, 0 }


struct command_line_option {
	const char *long_opt;
	const char *short_opt;
	int has_arg;
};


struct option_parser_data {
	/* static data (only stored, but remain unchanged) */
	int argc;
	const char **argv;
	const struct command_line_option *options;

	/* public data */
	int index;              /* index of next argv string; useful in OPTIONS_TERMINATOR case */
	const char *option;     /* current option */
	const char *argument;   /* current option argument or empty string */

	/* internal data (shouldn't be used) */
	const char *aggregate;
	char buf[3];
};


void init_option_parser(struct option_parser_data *data, int argc, const char *argv[],
		const struct command_line_option *options);

int get_option(struct option_parser_data *data);

