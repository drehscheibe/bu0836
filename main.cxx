#include "bu0836.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;


static void help(void)
{
	cout << "Usage:  bu0836 [<options>] ..." << endl;
	cout << "  -h, --help           show this help screen and exit" << endl;
	cout << "      --version        show version number and exit" << endl;
	cout << "  -v, --verbose        increase verbosity level" << endl;
	cout << endl;
	cout << "  -l, --list           list BU0836 devices (bus id, vendor, product, serial number, version)" << endl;
	cout << "  -d, --device <s>     select device by bus id or serial number (or significant substring thereof);" << endl;
	cout << "                       not needed when only one device is attached" << endl;
	cout << "  -m, --monitor        monitor device output" << endl;
	//cout << "  -i, --invert <n>     set inverted mode for given axis" << endl;
	//cout << "  -n, --normal <n>     set normal mode for given axis" << endl;
	//cout << "  -r, --rotary <n>     set rotary mode for given button (and its sibling)" << endl;
	//cout << "  -b, --button <n>     set button mode for given button (and its sibling)" << endl;
	cout << "  -O, --save <s>       save memory image to file <s>" << endl;
	cout << "  -I, --load <s>       load memory image from file <s>" << endl;
	cout << "  -X, --dump           show HID report and EEPROM image" << endl;
	cout << endl;
	cout << "Examples:" << endl;
	cout << "  $ bu0836 -l" << endl;
	cout << "                       ... to list available devices" << endl;
	cout << "  $ bu0836 -d2:4 -i0" << endl;
	cout << "                       ... invert first axis of device 2:4" << endl;
	exit(EXIT_SUCCESS);
}



static void version(void)
{
#ifdef GIT
	cout << ""STRINGIZE(GIT) << endl;
#endif
	exit(EXIT_SUCCESS);
}



int main(int argc, const char *argv[]) try
{
	enum { HELP_OPTION, VERSION_OPTION, VERBOSE_OPTION, LIST_OPTION, DEVICE_OPTION, MONITOR_OPTION, NORMAL_OPTION,
			INVERT_OPTION, BUTTON_OPTION, ROTARY_OPTION, SAVE_OPTION, LOAD_OPTION, DUMP_OPTION };

	const struct command_line_option options[] = {
		{ "--help",    "-h", 0, "\0" },
		{ "--version",    0, 0, "\0" },
		{ "--verbose", "-v", 0, "\0" },
		{ "--list",    "-l", 0, "\0" },
		{ "--device",  "-d", 1, "\0" },
		{ "--monitor", "-m", 0, "\1" },
		{ "--normal",  "-n", 1, "\1" },
		{ "--invert",  "-i", 1, "\1" },
		{ "--button",  "-b", 1, "\1" },
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
	while ((option = get_option(&ctx)) != OPTIONS_DONE)
		if (option == HELP_OPTION)
			help();
		else if (option == VERSION_OPTION)
			version();
		else if (option == VERBOSE_OPTION)
			set_log_level(get_log_level() - 1);

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

		case LIST_OPTION:
			for (size_t i = 0; i < dev.size(); i++) {
				const char *marker = &dev[i] == selected ? " <<" : "";
				cout << color("33") << dev[i].bus_address() << color()
						<< "  " << dev[i].manufacturer()
						<< ", " << dev[i].product()
						<< ", " << color("33") << dev[i].serial() << color()
						<< ", v" << dev[i].release()
						<< color("32") << marker << color()
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

		case BUTTON_OPTION:
			log(INFO) << "setting up button " << ctx.argument << " for button function" << endl;
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
			log(INFO) << "dumping HID report and EEPROM image" << endl;
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
