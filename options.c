#include <string.h>
#include "options.h"


int parse_options(int argc, const char *argv[], struct Options *options, int (*handler)(int index, const char *arg))
{
	const char *arg, *aggregate = "";
	char buf[3] = "-?";
	int index, found;

	if (argc < 2)
		return argc;

	for (index = 1; index < argc || *aggregate; ) {
		struct Options *opt = options;
		const char *optarg = 0;
		int grab_arg = 0;

		if (*aggregate) {
			buf[1] = *aggregate++;
			arg = buf;

		} else {
			arg = argv[index++];
			if (arg[0] != '-') {
				if (handler(OPTIONS_ARGUMENT, arg) == OPTIONS_ABORT)
					return --index;
				continue;
			}

			if (!strcmp(arg, "--"))
				break;

			if (arg[0] == '-' && (arg[1] && arg[1] != '-')) {
				aggregate = &arg[1];
				continue;
			}
		}

		for (found = 0; opt->long_opt || opt->short_opt; opt++, found++) {
			int len;
			if (opt->short_opt && !strcmp(opt->short_opt, arg)) {
				if (opt->has_arg) {
					if (*aggregate) {
						optarg = aggregate;
						aggregate = "";
					} else {
						grab_arg = 1;
					}
				}
				break;
			} else if (opt->long_opt && !strncmp(opt->long_opt, arg, len = strlen(opt->long_opt)) && arg[len] == '=') {
				if (opt->has_arg)
					optarg = arg + strlen(opt->long_opt) + 1;
				else if (handler(OPTIONS_EXCESS_ARGUMENT, arg) == OPTIONS_ABORT)
					return index;
				break;
			} else if (opt->long_opt && !strcmp(opt->long_opt, arg)) {
				if (opt->has_arg)
					grab_arg = 1;
				break;
			}
		}

		if (!opt->long_opt && !opt->short_opt) {
			if (handler(OPTIONS_UNKNOWN_OPTION, arg) == OPTIONS_ABORT)
				return index;
			continue;
		}

		if (grab_arg) {
			if (index < argc) {
				optarg = argv[index++];
			} else {
				if (handler(OPTIONS_MISSING_ARGUMENT, arg) == OPTIONS_ABORT)
					return index;
				optarg = "";
			}
		}

		if (handler(found, optarg) == OPTIONS_ABORT)
			return OPTIONS_ABORT;
	}

	if (!strcmp(arg, "--") && handler(OPTIONS_TERMINATOR, arg) == OPTIONS_CONTINUE) {
		while (index < argc && handler(OPTIONS_ARGUMENT, argv[index]) == OPTIONS_CONTINUE)
			index++;
	}
	return index;
}
