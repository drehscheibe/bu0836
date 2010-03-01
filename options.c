#include <string.h>
#include "options.h"


int parse_options(int argc, const char *argv[], struct Options *options, int (*handler)(int index, const char *arg))
{
	const char *arg, *shrt = "";
	char buf[3] = "-\0";

	int a;
	for (a = 1; a < argc; ) {
		if (*shrt) {
			buf[1] = *shrt++;
			arg = buf;

		} else {
			arg = argv[a];
			if (arg[0] != '-')
				break;

			a++;
			if (!strcmp(arg, "--"))
				break;

			if (arg[0] == '-' && (arg[1] && arg[1] != '-')) {
				shrt = &arg[1];
				continue;
			}
		}

		struct Options *opt = options;
		unsigned int o;
		int grab_arg = 0;
		const char *optarg = 0;  // FIXME initialize?

		for (o = 0; opt->long_opt || opt->short_opt; opt++, o++) {
			if (opt->short_opt && !strcmp(opt->short_opt, arg)) {
				if (opt->has_arg) {
					if (*shrt) {
						optarg = shrt;
						shrt = "";
					} else {
						grab_arg = 1;
					}
				}
				break;
			} else if (opt->long_opt && !strncmp(opt->long_opt, arg, strlen(opt->long_opt)) && arg[strlen(opt->long_opt)] == '=') {
				if (opt->has_arg)
					optarg = arg + strlen(opt->long_opt) + 1;
				else if (handler(OPTIONS_EXCESS_ARGUMENT, arg) == OPTIONS_ABORT)
					return a;
				break;
			} else if (opt->long_opt && !strcmp(opt->long_opt, arg)) {
				if (opt->has_arg)
					grab_arg = 1;
				break;
			}
		}

		if (!opt->long_opt) {
			if (handler(OPTIONS_UNKNOWN, arg) == OPTIONS_ABORT)
				return a;
			continue;
		}

		if (grab_arg) {
			if (a < argc) {
				optarg = argv[a++];
			} else {
				if (handler(OPTIONS_MISSING_ARGUMENT, arg) == OPTIONS_ABORT)
					return a;
				optarg = "";
			}
		}

		if (handler(o, optarg) == OPTIONS_ABORT)
			return OPTIONS_ABORT;
	}

	if (!strcmp(arg, "--") && handler(OPTIONS_TERMINATOR, 0) == OPTIONS_CONTINUE) {
		while (a < argc && handler(OPTIONS_ARGUMENT, argv[a]) == OPTIONS_CONTINUE)
			a++;
	}
	return a;
}
