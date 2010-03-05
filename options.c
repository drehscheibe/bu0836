#include <string.h>
#include "options.h"


void init_option_parser(struct option_parser_data *data, int argc, const char *argv[],
		const struct command_line_option *options)
{
	data->argc = argc;
	data->argv = argv;
	data->options = options;
	data->index = 1;
	data->option = data->argument = data->aggregate = "";
	data->buf[0] = '-', data->buf[1] = ' ', data->buf[2] = '\0';
}


int get_option(struct option_parser_data *data)
{
	const struct command_line_option *opt;
	int grab_arg, found;

	data->argument = "";

	if (data->argc < 2)
		return OPTIONS_DONE;

	if (!data->argv[data->index] && !*data->aggregate)
		return OPTIONS_DONE;

	if (data->buf[1] == '-') {
		data->option = data->argv[data->index++];
		return OPTIONS_ARGUMENT;
	}

	if (*data->aggregate) {
		data->buf[1] = *data->aggregate++;
		data->option = data->buf;

	} else {
		data->option = data->argv[data->index++];
		if (data->option[0] != '-')
			return OPTIONS_ARGUMENT;

		if (!data->option[1])
			return OPTIONS_ARGUMENT;

		if (data->option[1] == '-') {
			if (!data->option[2]) {
				data->buf[1] = '-';
				return OPTIONS_TERMINATOR;
			}
		} else {
			data->aggregate = &data->option[1];
			return get_option(data);
		}
	}

	for (grab_arg = found = 0, opt = data->options; opt->long_opt || opt->short_opt; opt++, found++) {
		int len;
		if (opt->short_opt && !strcmp(opt->short_opt, data->option)) {
			if (opt->has_arg) {
				if (*data->aggregate) {
					data->argument = data->aggregate;
					data->aggregate = "";
				} else {
					grab_arg = 1;
				}
			}
			break;

		} else if (opt->long_opt && !strncmp(opt->long_opt, data->option, len = strlen(opt->long_opt))
				&& data->option[len] == '=') {
			if (opt->has_arg) {
				data->argument = data->option + strlen(opt->long_opt) + 1;
				data->option = opt->long_opt;
			} else {
				return OPTIONS_EXCESS_ARGUMENT;
			}
			break;

		} else if (opt->long_opt && !strcmp(opt->long_opt, data->option)) {
			if (opt->has_arg)
				grab_arg = 1;
			break;
		}
	}

	if (!opt->long_opt && !opt->short_opt)
		return OPTIONS_UNKNOWN_OPTION;

	if (grab_arg) {
		if (data->argv[data->index]) {
			data->argument = data->argv[data->index++];
		} else {
			data->argument = "";
			return OPTIONS_MISSING_ARGUMENT;
		}
	}

	return found;
}
