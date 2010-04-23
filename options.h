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
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

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

	struct option_parser_context ctx;
	init_options_context(&ctx, argc, argv, options);

	int option;
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {
		switch (option) {
		case 0: cerr << "HELP" << endl; break;
		case 1: cerr << "LONG" << endl; break;
		case 2: cerr << "SHORT" << endl; break;
		case 3: cerr << "FILE " << ctx.argument << endl; break;
		case OPTIONS_ARGUMENT: cerr << "non-option argument " << ctx.argument << endl; break;
		case OPTIONS_TERMINATOR: break;         // ignore "--"
		default: cerr << "bad option" << endl;  // lazy handling of all other error codes
		}
	}


Remarks:
- Long options with optional arguments (--opt[=arg]) can be implemented by checking
  for ctx.option and ctx.argument in an OPTIONS_EXCESS_ARGUMENT switch case.

*/

#ifdef __cplusplus
extern "C" {
#endif

/* get_option() returns a "struct command_line_option" index, or one of the following codes */

#define OPTIONS_DONE -1                 /* parsing done; no more options available */
#define OPTIONS_TERMINATOR -2           /* "--" wass issued; remainder seen as arguments */
#define OPTIONS_ARGUMENT -3             /* argument found ("-" or string not starting with '-' */
#define OPTIONS_EXCESS_ARGUMENT -4      /* long option has argument, but shouldn't (e.g. --help=foo) */
#define OPTIONS_MISSING_ARGUMENT -5     /* last option wants argument, but there are none left */
#define OPTIONS_UNKNOWN_OPTION -6       /* undefined option found */

#define OPTIONS_LAST { 0, 0 }           /* can be used to terminate command_line_option array */


struct command_line_option {
	const char *long_opt;
	const char *short_opt;
	int has_arg;

	const char *ext;                /* unused; free for application extensions */
};


struct option_parser_context {
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


void init_options_context(struct option_parser_context *ctx, int argc, const char *argv[],
		const struct command_line_option *options);

int get_option(struct option_parser_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
