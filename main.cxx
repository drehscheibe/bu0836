#include <sstream>

#include "bu0836.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;



namespace {

void help(void)
{
	cout << "Usage:  bu0836 [<options>] ..." << endl;
	cout << "  -h, --help           show this help screen and exit" << endl;
	cout << "      --version        show version number and exit" << endl;
	cout << "  -v, --verbose        increase verbosity level" << endl;
	cout << endl;
	cout << "  -l, --list           list BU0836 devices (bus id, vendor, product, serial number, version)" << endl;
	cout << "  -d, --device <s>     select device by bus id or serial number (or significant ending thereof);" << endl;
	cout << "                       not needed if only one device is attached" << endl;
	cout << "  -a, --axes <list>    select axes (comma separated numbers with optional ranges, e.g. 0,2,4-6)" << endl;
	cout << "  -b, --buttons <list> select buttons" << endl;
	cout << "  -m, --monitor        monitor device output (terminate with Ctrl-c)" << endl;
	cout << "  -O, --save <s>       save memory image to file <s>" << endl;
	cout << "  -I, --load <s>       load memory image from file <s>" << endl;
	cout << "  -X, --dump           display EEPROM image" << endl;
	cout << endl;
	cout << "Examples:" << endl;
	cout << "  $ bu0836 -l" << endl;
	cout << "                       ... to list available devices" << endl;
	cout << "  $ bu0836 -d2:4 -i0" << endl;
	cout << "                       ... invert first axis of device 2:4" << endl;
}



void version(void)
{
#ifdef GIT
	cout << ""STRINGIZE(GIT) << endl;
#else
	cout << "0.0" << endl;
#endif
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

} // namespace



int main(int argc, const char *argv[]) try
{
	enum {
		HELP_OPTION, VERSION_OPTION, VERBOSE_OPTION, LIST_OPTION, DEVICE_OPTION,
		AXIS_OPTION, BUTTON_OPTION, MONITOR_OPTION, NORMAL_OPTION,
		INVERT_OPTION, ROTARY_OPTION, SAVE_OPTION, LOAD_OPTION, DUMP_OPTION,
	};

	const struct command_line_option options[] = {
		{ "--help",    "-h", 0, "\0" },
		{ "--version",    0, 0, "\0" },
		{ "--verbose", "-v", 0, "\0" },
		{ "--list",    "-l", 0, "\0" },
		{ "--device",  "-d", 1, "\0" },
		{ "--axes",    "-a", 1, "\0" },
		{ "--buttons", "-b", 1, "\0" },
		{ "--monitor", "-m", 0, "\1" },
		{ "--normal",  "-n", 1, "\1" },
		{ "--invert",  "-i", 1, "\1" },
		{ "--rotary",  "-r", 1, "\1" },
		{ "--save",    "-O", 1, "\1" },
		{ "--load",    "-I", 1, "\1" },
		{ "--dump",    "-X", 0, "\1" },
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
	controller *selected = 0;

	int numdev = dev.size();
	if (numdev == 1)
		selected = &dev[0];
	else if (!numdev)
		throw string("no BU0836* found");

	// second pass options
	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {

		if (option >= 0 && options[option].ext[0]) {
			if (!selected)
				throw string("you need to select a device before you can use the ") + options[option].long_opt
						+ " option, for\n       example with -d" + dev[0].bus_address() + " or -d"
						+ dev[0].serial() + ". Use the --list option for available devices.";

			selected->claim();
		}

		switch (option) {
		case DEVICE_OPTION: {
			int num = dev.find(ctx.argument, &selected);
			if (num == 1)
				log(INFO) << "selecting device '" << selected->serial() << '\'' << endl;
			else if (num)
				throw string("ambiguous device specifier");
			else
				throw string("no matching device found");
			break;
		}

		case AXIS_OPTION:
			log(INFO) << "selecting axes " << hex << numlist_to_bitmap(ctx.argument, 7) << dec << endl;
			break;

		case BUTTON_OPTION:
			log(INFO) << "selecting buttons " << hex << numlist_to_bitmap(ctx.argument, 31) << dec << endl;
			break;


		case LIST_OPTION:
			for (size_t i = 0; i < dev.size(); i++) {
				const char *marker = &dev[i] == selected ? " <<" : "";
				cout << YELLOW << dev[i].bus_address() << NORM
						<< "  " << dev[i].manufacturer()
						<< ", " << dev[i].product()
						<< ", " << YELLOW << dev[i].serial() << NORM
						<< ", v" << dev[i].release()
						<< GREEN << marker << NORM
						<< endl;
			}
			break;

		case MONITOR_OPTION:
			log(INFO) << "monitoring" << endl;
			selected->show_input_reports();
			break;

		case NORMAL_OPTION:
			log(INFO) << "setting axis " << ctx.argument << " to normal" << endl;
			break;

		case INVERT_OPTION:
			log(INFO) << "setting axis " << ctx.argument << " to inverted" << endl;
			break;

		case ROTARY_OPTION:
			log(INFO) << "setting up button " << ctx.argument << " for rotary switch" << endl;
			break;

		case SAVE_OPTION:
			log(INFO) << "save image to file '" << ctx.argument << '\'' << endl;
			if (!selected->get_image() && !selected->save_image(ctx.argument))
				log(INFO) << "saved" << endl;
			break;

		case LOAD_OPTION:
			log(INFO) << "load image from file '" << ctx.argument << '\'' << endl;
			if (!selected->load_image(ctx.argument)) // && !selected->set_image()
				log(INFO) << "loaded" << endl;
			break;

		case DUMP_OPTION:
			log(INFO) << "EEPROM image" << endl;
			selected->dump_internal_data();

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
	return EXIT_SUCCESS;

} catch (const string &msg) {
	log(ALERT) << "Error: " << msg << endl;
	return EXIT_FAILURE;
}
