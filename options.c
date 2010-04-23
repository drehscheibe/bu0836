// command line options parser
//
// Copyright (C) 2010  Melchior FRANZ  <melchior.franz@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#include <string.h>
#include "options.h"


void init_options_context(struct option_parser_context *ctx, int argc, const char *argv[],
		const struct command_line_option *options)
{
	ctx->argc = argc;
	ctx->argv = argv;
	ctx->options = options;
	ctx->index = 1;
	ctx->option = ctx->argument = ctx->aggregate = "";
	ctx->buf[0] = '-', ctx->buf[1] = ' ', ctx->buf[2] = '\0';
}


int get_option(struct option_parser_context *ctx)
{
	const struct command_line_option *opt;
	int grab_arg, found;

	ctx->argument = "";

	if (ctx->argc < 2)
		return OPTIONS_DONE;

	if (!ctx->argv[ctx->index] && !*ctx->aggregate)
		return OPTIONS_DONE;

	if (ctx->buf[1] == '-') {
		ctx->option = ctx->argv[ctx->index++];
		return OPTIONS_ARGUMENT;
	}

	if (*ctx->aggregate) {
		ctx->buf[1] = *ctx->aggregate++;
		ctx->option = ctx->buf;

	} else {
		ctx->option = ctx->argv[ctx->index++];
		if (ctx->option[0] != '-')
			return OPTIONS_ARGUMENT;

		if (!ctx->option[1])
			return OPTIONS_ARGUMENT;

		if (ctx->option[1] == '-') {
			if (!ctx->option[2]) {
				ctx->buf[1] = '-';
				return OPTIONS_TERMINATOR;
			}
		} else {
			ctx->aggregate = &ctx->option[1];
			return get_option(ctx);
		}
	}

	for (grab_arg = found = 0, opt = ctx->options; opt->long_opt || opt->short_opt; opt++, found++) {
		int len;
		if (opt->short_opt && !strcmp(opt->short_opt, ctx->option)) {
			if (opt->has_arg) {
				if (*ctx->aggregate) {
					ctx->argument = ctx->aggregate;
					ctx->aggregate = "";
				} else {
					grab_arg = 1;
				}
			}
			break;

		} else if (opt->long_opt && !strncmp(opt->long_opt, ctx->option, len = strlen(opt->long_opt))
				&& ctx->option[len] == '=') {
			ctx->argument = ctx->option + strlen(opt->long_opt) + 1;
			ctx->option = opt->long_opt;
			if (!opt->has_arg)
				return OPTIONS_EXCESS_ARGUMENT;
			break;

		} else if (opt->long_opt && !strcmp(opt->long_opt, ctx->option)) {
			if (opt->has_arg)
				grab_arg = 1;
			break;
		}
	}

	if (!opt->long_opt && !opt->short_opt)
		return OPTIONS_UNKNOWN_OPTION;

	if (grab_arg) {
		if (ctx->argv[ctx->index]) {
			ctx->argument = ctx->argv[ctx->index++];
		} else {
			ctx->argument = "";
			return OPTIONS_MISSING_ARGUMENT;
		}
	}

	return found;
}
