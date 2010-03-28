#include "bu0836.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;


static void help(void)
{
	cout << "Usage:  bu0836 [-n <number>] [-i <number>] ..." << endl;
	cout << "  -h, --help           this help screen" << endl;
	cout << "  -v, --verbose        increase verbosity level" << endl;
	cout << "  -l, --list           list BU0836 controller devices (bus id, vendor, product, serial no., version)" << endl;
	cout << "  -d, --device <s>     select device by bus id or serial number (or significant substring thereof)" << endl;
	cout << "                       (not needed if only one device is attached)" << endl;
	cout << "  -m, --monitor        monitor device output" << endl;
	cout << "  -i, --invert <n>     set inverted mode for given axis" << endl;
	cout << "  -n, --normal <n>     set normal mode for given axis" << endl;
	cout << "  -r, --rotary <n>     set rotary mode for given button (and its sibling)" << endl;
	cout << "  -b, --button <n>     set button mode for given button (and its sibling)" << endl;
	cout << "  -O, --save <s>       save memory image to file <s>" << endl;
	cout << "  -I, --load <s>       load memory image from file <s>" << endl;
	cout << endl;
	cout << "Examples:" << endl;
	cout << "  $ bu0836 -l" << endl;
	cout << "                       ... to list available devices" << endl;
	cout << "  $ bu0836 -d2:4 -i0" << endl;
	cout << "                       ... invert first axis of device 2:4" << endl;
#ifdef GIT
	cout << endl;
	cout << "Version:" << endl;
	cout << "  "STRINGIZE(GIT) << endl;
#endif
	exit(EXIT_SUCCESS);
}



int main(int argc, const char *argv[]) try
{
	enum { HELP_OPTION, VERBOSE_OPTION, LIST_OPTION, DEVICE_OPTION, MONITOR_OPTION, NORMAL_OPTION,
			INVERT_OPTION, BUTTON_OPTION, ROTARY_OPTION, SAVE_OPTION, LOAD_OPTION };

	const struct command_line_option options[] = {
		{ "--help",    "-h", 0, "\0" },
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
		OPTIONS_LAST
	};

	int option;
	struct option_parser_context ctx;

	// first pass options
	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE)
		if (option == HELP_OPTION)
			help();
		else if (option == VERBOSE_OPTION)
			set_log_level(get_log_level() + 1);

	bu0836 usb;
	controller *selected = 0;

	int numdev = usb.size();
	if (numdev == 1)
		selected = &usb[0];
	else if (!numdev)
		throw string("no BU0836* found");

	// second pass options
	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {

		if (option >= 0 && options[option].ext[0]) {
			if (!selected)
				throw string("you need to select a device before you can use the ") + options[option].long_opt + " option";

			selected->claim();
		}

		switch (option) {
		case DEVICE_OPTION: {
			int num = usb.find(ctx.argument, &selected);
			if (num == 1)
				log(INFO) << "selecting device '" << selected->serial() << '\'' << endl;
			else if (num)
				log(ALERT) << "ambiguous device specifier (" << num << " devices matching)" << endl;
			else
				log(ALERT) << "no matching device found" << endl;
			break;
		}

		case LIST_OPTION:
			for (size_t i = 0; i < usb.size(); i++)
				usb[i].print_info();
			break;

		case MONITOR_OPTION:
			log(INFO) << "monitoring" << endl;
			selected->get_data();
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

		case HELP_OPTION:
		case VERBOSE_OPTION:     // already handled in first pass

		// signals and errors
		case OPTIONS_TERMINATOR:
			break;

		case OPTIONS_ARGUMENT:
			log(ALERT) << "ERROR: don't know what to do with an argument '" << ctx.option << '\'' << endl;
			return EXIT_FAILURE;

		case OPTIONS_EXCESS_ARGUMENT:
			log(ALERT) << "ERROR: illegal option assignment '" << ctx.argument << '\'' << endl;
			return EXIT_FAILURE;

		case OPTIONS_UNKNOWN_OPTION:
			log(ALERT) << "ERROR: unknown option '" << ctx.option << '\'' << endl;
			return EXIT_FAILURE;

		case OPTIONS_MISSING_ARGUMENT:
			log(ALERT) << "ERROR: missing argument for option '" << ctx.option << '\'' << endl;
			return EXIT_FAILURE;

		default:
			log(ALERT) << color("31;1") << "this can't happen: " << option << '/' << ctx.option << color() << endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;

} catch (string &msg) {
	log(ALERT) << "Error: " << msg << endl;
	return EXIT_FAILURE;
}
