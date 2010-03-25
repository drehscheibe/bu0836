#include "bu0836a.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;


static void help(void)
{
	cout << "Usage:  bu0836a [-n <number>] [-i <number>] ..." << endl;
	cout << "        -h, --help           this help screen" << endl;
	cout << "        -v, --verbose        increase verbosity level" << endl;
	cout << "        -l, --list           list BU0836 controller devices" << endl;
	cout << "        -m, --monitor        monitor device output" << endl;
	cout << "        -d, --device <s>     select device by serial number (or unambiguous substring)" << endl;
	cout << "        -i, --invert <n>     set inverted mode for given axis" << endl;
	cout << "        -n, --normal <n>     set normal mode for given axis" << endl;
	cout << "        -r, --rotary <n>     set rotary mode for given button (and its sibling)" << endl;
	cout << "        -b, --button <n>     set button mode for given button (and its sibling)" << endl;
	exit(EXIT_SUCCESS);
}



int main(int argc, const char *argv[]) try
{
	enum { HELP_OPTION, VERBOSE_OPTION, DEVICE_OPTION, LIST_OPTION, MONITOR_OPTION, NORMAL_OPTION,
			INVERT_OPTION, BUTTON_OPTION, ROTARY_OPTION };

	const struct command_line_option options[] = {
		{ "--help", "-h", 0 },
		{ "--verbose", "-v", 0 },
		{ "--device", "-d", 1 },
		{ "--list", "-l", 0 },
		{ "--monitor", "-m", 0 },
		{ "--normal", "-n", 1 },
		{ "--invert", "-i", 1 },
		{ "--button", "-b", 1 },
		{ "--rotary", "-r", 1 },
		OPTIONS_LAST
	};

	int option;
	struct option_parser_context ctx;

	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE)
		if (option == HELP_OPTION)
			help();

	bu0836a usb;
	controller *selected = 0;

	int numdev = usb.size();
	if (numdev == 1)
		selected = &usb[0];
	else if (!numdev)
		throw string("no BU0836A found");

	init_options_context(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {
		switch (option) {
		case VERBOSE_OPTION:
			set_log_level(get_log_level() + 1);
			break;

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
			if (selected)
				selected->get_data();
			else
				log(ALERT) << "no device selected" << endl;
			break;

		case NORMAL_OPTION:
			log(INFO) << "setting axis " << ctx.argument << " to normal" << endl;
			break;

		case INVERT_OPTION:
			if (selected)
				log(INFO) << "setting axis " << ctx.argument << " to inverted" << endl;
			else
				log(ALERT) << "you have to select a device first" << endl;
			break;

		case BUTTON_OPTION:
			log(INFO) << "setting up button " << ctx.argument << " for button function" << endl;
			break;

		case ROTARY_OPTION:
			log(INFO) << "setting up button " << ctx.argument << " for rotary switch" << endl;
			break;

		case HELP_OPTION:

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
			log(ALERT) << "\033[31;1mthis can't happen: " << option << "/" << ctx.option << "\033[m" << endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;

} catch (string &msg) {
	log(ALERT) << "Error: " << msg << endl;
	return EXIT_FAILURE;
}
