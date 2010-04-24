// configuration utility for bu0836 joystick controllers
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
#include <sstream>

#include "bu0836.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;
using namespace logging;



namespace {

void help(void)
{
	cout << "Usage:  bu0836 [<options>]" << endl;
	cout << endl;
	cout << "  -h, --help             show this help screen and exit" << endl;
	cout << "      --version          show version number and exit" << endl;
	cout << "  -v, --verbose          increase verbosity level" << endl;
	cout << "  -l, --list             list BU0836 devices (bus id, vendor, product, serial number, version)" << endl;
	cout << "  -d, --device <s>       select device by bus id or serial number (or significant ending thereof);" << endl;
	cout << "                         not needed if only one device is attached" << endl;
	cout << endl;
	cout << "  -s, --status           show current device configuration" << endl;
	cout << "  -m, --monitor          monitor device output (terminate with Ctrl-c)" << endl;
	cout << "  -r, --reset            reset device configuration to \"factory default\"" << endl;
	cout << "                         (equivalent of --axes=0-7 --invert=0 --zoom=0 --buttons=0-31 --encoder=0)" << endl;
	cout << "  -y, --sync             write current changes to the controller's EEPROM" << endl;
	cout << "  -O, --save <s>         save EEPROM image to file <s>" << endl;
	cout << "  -I, --load <s>         load EEPROM image from file <s>" << endl;
	cout << "  -X, --dump             display EEPROM image" << endl;
	cout << endl;
	cout << "  -a, --axes <list>      select axes (overrides prior axis selection)" << endl;
	cout << "  -i, --invert <n>       set selected axes to inverted (n=1) or normal (n=0) mode" << endl;
	cout << "  -z, --zoom <n>         set zoom factor for selected axes" << endl;
	cout << endl;
	cout << "  -b, --buttons <list>   select buttons (overrides prior button selection)" << endl;
	cout << "  -e, --encoder <s>      set encoder mode for selected buttons (and their associated siblings):" << endl;
	cout << "                         \"0\" or \"off\" for normal button function," << endl;
	cout << "                         \"1\" or \"1:1\" for quarter wave encoder," << endl;
	cout << "                         \"2\" or \"1:2\" for half wave encoder," << endl;
	cout << "                         \"3\" or \"1:4\" for full wave encoder" << endl;
	cout << "  -p, --pulse-width <n>  set pulse width for all encoders" << endl;
	cout << endl << endl;
	cout << "  <list> is a number or number range, or a list thereof separated by commas," << endl;
	cout << "         e.g. \"0\" or \"1,3,5\" or \"0-5\" or \"0,2,6-10,30\"" << endl;
	cout << endl << endl;
	cout << "Examples:" << endl;
	cout << "  $ bu0836 -l" << endl;
	cout << "                         ... list available devices" << endl;
	cout << "  $ bu0836 -d2:4 -a0,2 -i1" << endl;
	cout << "                         ... invert first and third axis of device with USB bus address 2:4" << endl;
	cout << "  $ bu0836 -dA12136 -a0-7 -z0" << endl;
	cout << "                         ... turn zoom off for all axes of device with serial number A12136" << endl;
	cout << "  $ bu0836 -d36 -b4 -e1:2" << endl;
	cout << "                         ... configure buttons 4 and 5 of device (whose serial" << endl;
	cout << "                             number ends with) 36 for half wave encoder" << endl;
}



void version(void)
{
#ifdef TAG
	cout << "bu0836  v"STRINGIZE(TAG)"  ("STRINGIZE(SHA)")";
#if MOD
	cout << "++";
#endif
	cout << endl;
#else
	cout << "??" << endl;
#endif
	cout << "Copyright (C)  Melchior FRANZ  <melchior.franz@gmail.com>" << endl;
}



uint32_t numlist_to_bitmap(const char *list, unsigned int max = 31)
{
	uint32_t m = 0;
	bool range = false;
	const unsigned undef = ~0;
	unsigned low = undef, high = undef;
	istringstream s(list);

	while (1) {
		int next = s.get();
		if (s.eof() || next == ',') {
			if (range)
				throw string("incomplete range in number list");

			if (low != undef) {
				if (high == undef)
					high = low;
				for (unsigned i = low; i <= high; i++)
					m |= 1 << i;
			}

			if (s.eof())
				break;

			low = high = undef;

		} else if (isdigit(next)) {
			s.putback(next);
			unsigned i;
			s >> i;
			if (i > max) {
				log(WARN) << "number in axis/button list out of range: " << i << " (max: " << max << ')' << endl;
				i = max;
			}

			if (!range && low == undef)
				low = i;
			else if (range && high == undef)
				high = i;
			else
				throw string("unexpected number in list");
			range = false;

		} else if (next == '-') {
			if (low == undef || range)
				throw string("unexpected range in number list");
			range = true;

		} else if (!isspace(next)) {
			throw string("malformed number list");
		}
	}
	return m;
}



void list_devices(bu0836& dev)
{
	for (size_t i = 0; i < dev.size(); i++) {
		const char *marker = &dev[i] == dev.selected() ? " <<" : "";
		cout << brown << dev[i].bus_address() << reset
				<< "\t" << dev[i].manufacturer()
				<< ", " << dev[i].product()
				<< ", " << brown << dev[i].serial() << reset
				<< ", v" << dev[i].release()
				<< green << marker << reset
				<< endl;
	}
}



void commit_changes(bu0836& dev)
{
	for (size_t i = 0; i < dev.size(); i++) {
		if (!dev[i].is_dirty())
			continue;

		cerr << endl << endl << endl << endl;
		dev[i].print_status();
		int key;
		do {
			cerr << cyan << "Write configuration to controller? [Y/n] " << reset;
			key = cin.get();
			cin.clear();
			if (key == '\n')
				key = 'y';
			else
				cin.ignore(80, cin.widen('\n'));
		} while (!cin.fail() && key != 'n' && key != 'N' && key != 'y' && key != 'Y');

		if (key == 'y' || key == 'Y')
			dev[i].sync();
	}
}

} // namespace



int main(int argc, const char *argv[]) try
{
	enum {
		HELP_OPTION, VERSION_OPTION, VERBOSE_OPTION,
		LIST_OPTION, DEVICE_OPTION, STATUS_OPTION, MONITOR_OPTION,
		RESET_OPTION, SYNC_OPTION, SAVE_OPTION, LOAD_OPTION, DUMP_OPTION,
		AXES_OPTION, INVERT_OPTION, ZOOM_OPTION,
		BUTTONS_OPTION, ENCODER_OPTION, PULSEWIDTH_OPTION,
	};

	const struct command_line_option options[] = {
		{ "--help",        "-h", 0, "\0" },
		{ "--version",        0, 0, "\0" },
		{ "--verbose",     "-v", 0, "\0" },
		{ "--list",        "-l", 0, "\0" },
		{ "--device",      "-d", 1, "\0" },
		//
		{ "--status",      "-s", 0, "d" },
		{ "--monitor",     "-m", 0, "d" },
		{ "--reset",       "-r", 0, "d" },
		{ "--sync",        "-y", 0, "d" },
		{ "--save",        "-O", 1, "d" },
		{ "--load",        "-I", 1, "d" },
		{ "--dump",        "-X", 0, "d" },
		//
		{ "--axes",        "-a", 1, "\0" },
		{ "--invert",      "-i", 1, "a" },
		{ "--zoom",        "-z", 1, "a" },
		//
		{ "--buttons",     "-b", 1, "\0" },
		{ "--encoder",     "-e", 1, "b" },
		{ "--pulse-width", "-p", 1, "\0" },
		OPTIONS_LAST
	};

	int option;
	struct option_parser_context ctx;

	// first pass options
	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {
		if (option == HELP_OPTION) {
			help();
			return EXIT_SUCCESS;

		} else if (option == VERSION_OPTION) {
			version();
			return EXIT_SUCCESS;

		} else if (option == VERBOSE_OPTION) {
			set_log_level(get_log_level() - 1);
		}
	}

	bu0836 dev;
	if (dev.empty())
		throw string("no BU0836* found");

	uint32_t selected_axes = 0;
	uint32_t selected_buttons = 0;

	// second pass options
	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {

		// check for option dependencies
		if (option >= 0) {
			if (options[option].ext[0]) {
				if (!dev.selected())
					throw string("you need to select a device before you can use the ") + options[option].long_opt
							+ " option, for\n       example with -d" + dev[0].bus_address() + " or -d"
							+ dev[0].serial() + ". Use the --list option for available devices.";
				dev.selected()->claim();
			}
			if (options[option].ext[0] == 'a' && !selected_axes)
				throw string("no axes selected for ") + options[option].long_opt + " option";
			if (options[option].ext[0] == 'b' && !selected_buttons)
				throw string("no buttons selected for ") + options[option].long_opt + " option";
		}

		switch (option) {
		case LIST_OPTION:
			list_devices(dev);
			break;

		case DEVICE_OPTION: {
			int num = dev.select(ctx.argument);
			if (num == 1)
				log(INFO) << "selecting device '" << dev.selected()->serial() << '\'' << endl;
			else if (num)
				throw string("ambiguous device specifier");
			else
				throw string("no matching device found");
			break;
		}

		case STATUS_OPTION:
			dev.selected()->print_status();
			break;

		case MONITOR_OPTION:
			dev.selected()->show_input_reports();
			break;

		case RESET_OPTION:
			log(INFO) << "resetting configuration to \"factory default\"" << endl;
			for (int i = 0; i < 8; i++) {
				dev.selected()->set_invert(i, false);
				dev.selected()->set_zoom(i, 0);
			}
			for (int i = 0; i < 32; i += 2)
				dev.selected()->set_encoder_mode(i, 0);
			dev.selected()->set_pulse_width(6);
			break;

		case SYNC_OPTION:
			log(INFO) << "write changes to EEPROM" << endl;
			dev.selected()->sync();
			break;

		case SAVE_OPTION:
			log(INFO) << "saving image to file '" << ctx.argument << '\'' << endl;
			if (!dev.selected()->get_eeprom() && !dev.selected()->save_image(ctx.argument))
				log(INFO) << "saved" << endl;
			break;

		case LOAD_OPTION:
			log(INFO) << "loading image from file '" << ctx.argument << '\'' << endl;
			if (!dev.selected()->load_image(ctx.argument)) // && !dev.selected()->set_eeprom()
				log(INFO) << "loaded" << endl;
			break;

		case DUMP_OPTION:
			log(INFO) << "EEPROM image" << endl;
			dev.selected()->dump_internal_data();
			break;

		case AXES_OPTION:
			selected_axes = numlist_to_bitmap(ctx.argument, 7);
			log(INFO) << "selecting axes 0x" << hex << selected_axes << dec << endl;
			break;

		case INVERT_OPTION: {
			log(INFO) << "setting axes to inverted=" << ctx.argument << endl;
			string arg = ctx.argument;
			bool invert;
			if (arg == "0" || arg == "false")
				invert = false;
			else if (arg == "1" || arg == "true")
				invert = true;
			else
				throw string("invalid argument to --invert option; use 0/false and 1/true");
			for (uint32_t i = 0; i < 8; i++)
				if (selected_axes & (1 << i))
					dev.selected()->set_invert(i, invert);
			break;
		}

		case ZOOM_OPTION: {
			istringstream x(ctx.argument);
			unsigned zoom;
			x >> zoom;
			log(INFO) << "setting axes to zoom=" << zoom << endl;
			for (uint32_t i = 0; i < 8; i++)
				if (selected_axes & (1 << i))
					dev.selected()->set_zoom(i, zoom);
			break;
		}

		case BUTTONS_OPTION:
			selected_buttons = numlist_to_bitmap(ctx.argument, 31);
			log(INFO) << "selecting buttons 0x" << hex << selected_buttons << dec << endl;
			break;

		case ENCODER_OPTION: {
			string arg = ctx.argument;
			int enc;
			if (arg == "off" || arg == "false" || arg == "0")
				enc = 0;
			else if (arg == "1:1" || arg == "1")
				enc = 1;
			else if (arg == "1:2" || arg == "2")
				enc = 2;
			else if (arg == "1:4" || arg == "3")
				enc = 3;
			else
				throw string("bad --encoder option \"") + arg + "\" (use \"off\", \"1:1\", \"1:2\", or \"1:4\")";
			log(INFO) << "configuring buttons for encoder mode " << enc << endl;
			for (uint32_t i = 0; i < 31; i++)
				if (selected_buttons & (1 << i))
					dev.selected()->set_encoder_mode(i, enc);
			break;
		}

		case PULSEWIDTH_OPTION: {
			istringstream x(ctx.argument);
			unsigned i;
			x >> i;
			log(INFO) << "pulse width = " << i << " " << bool(x.eof()) << endl;
			break;
		}

		// ignored options
		case HELP_OPTION:
		case VERSION_OPTION:
		case VERBOSE_OPTION:

		// signals and errors
		case OPTIONS_TERMINATOR:
			break;

		case OPTIONS_ARGUMENT:
			throw string("don't know what to do with an argument '") + ctx.option + '\'';

		case OPTIONS_EXCESS_ARGUMENT:
			throw string("illegal option assignment '") + ctx.argument + '\'';

		case OPTIONS_UNKNOWN_OPTION:
			throw string("unknown option '") + ctx.option + '\'';

		case OPTIONS_MISSING_ARGUMENT:
			throw string("missing argument for option '") + ctx.option + '\'';

		default:
			log(ALERT) << "this can't happen: " << option << '/' << ctx.option << endl;
			return EXIT_FAILURE;
		}
	}

	commit_changes(dev);
	return EXIT_SUCCESS;

} catch (const string &msg) {
	log(ALERT) << "Error: " << msg << endl;
	return EXIT_FAILURE;
}
