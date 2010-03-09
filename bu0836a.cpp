#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <libusb.h>
#include "options.h"
#include "bu0836a.h"

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



class config {
public:
	static int verbosity;
};

int config::verbosity = 1;


#define ALWAYS -1
#define ALERT 0
#define WARN 1
#define INFO 2
#define BULK 3

namespace {
	class nullbuf : public streambuf { } nb;
	ostream cnull(&nb);
}


std::ostream& log(int log_level = ALWAYS, bool condition = true)
{
	return condition && log_level < config::verbosity ? cerr : cnull;
}



static const char *usb_perror(int errno)
{
	switch (errno) {
	case LIBUSB_SUCCESS:
		return "success";
	case LIBUSB_ERROR_IO:
		return "input/output error";
	case LIBUSB_ERROR_INVALID_PARAM:
		return "invalid parameter";
	case LIBUSB_ERROR_ACCESS:
		return "access denied (insufficient permissions)";
	case LIBUSB_ERROR_NO_DEVICE:
		return "no such device (it may have been disconnected)";
	case LIBUSB_ERROR_NOT_FOUND:
		return "entity not found";
	case LIBUSB_ERROR_BUSY:
		return "resource busy";
	case LIBUSB_ERROR_TIMEOUT:
		return "operation timed out";
	case LIBUSB_ERROR_OVERFLOW:
		return "overflow";
	case LIBUSB_ERROR_PIPE:
		return "pipe error";
	case LIBUSB_ERROR_INTERRUPTED:
		return "system call interrupted (perhaps due to signal)";
	case LIBUSB_ERROR_NO_MEM:
		return "insufficient memory";
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return "operation not supported or unimplemented on this platform";
	case LIBUSB_ERROR_OTHER:
		return "other error";
	default:
		return "unknown error code";
	}
}



static string bcd2str(int n)
{
	ostringstream o;
	o << hex << ((n >> 12) & 0xf) << ((n >> 8) & 0xf) << '.' << ((n >> 4) & 0xf) << (n & 0xf);
	return o.str();
}



static string strip(string s)
{
	const char spaces[] = " \t\n\r\b";
	string::size_type start = s.find_first_not_of(spaces, 0);
	string::size_type end = s.find_last_not_of(spaces, string::npos);
	return start == string::npos || end == string::npos ? "" : s.substr(start, end - start + 1);
}



class controller {
public:
	controller(libusb_device_handle *handle, libusb_device *device, libusb_device_descriptor desc) :
		_handle(handle),
		_device(device),
		_desc(desc),
		_claimed(false),
		_kernel_detached(false)
	{
		ostringstream s;
		s << int(libusb_get_bus_number(_device)) << ":" << int(libusb_get_device_address(_device));
		_bus_address = s.str();

		s.str("");
		s << hex << setw(4) << setfill('0') << _desc.idVendor << ':' << setw(4) << setfill('0') << _desc.idProduct;
		_id = s.str();

		_release = bcd2str(desc.bcdDevice);

		unsigned char buf[256];
		if (desc.iManufacturer && libusb_get_string_descriptor_ascii(_handle, desc.iManufacturer, buf, sizeof(buf)) > 0)
			_manufacturer = strip(string((char *)buf));

		if (desc.iProduct && libusb_get_string_descriptor_ascii(_handle, desc.iProduct, buf, sizeof(buf)) > 0)
			_product = strip(string((char *)buf));

		if (desc.iSerialNumber && libusb_get_string_descriptor_ascii(_handle, desc.iSerialNumber, buf, sizeof(buf)) > 0)
			_serial = strip(string((char *)buf));

		_jsid = _manufacturer;
		if (!_manufacturer.empty() && !_product.empty())
			_jsid += " ";
		_jsid += _product;
		if (!_jsid.empty() && !_serial.empty())
			_jsid += " ";
		_jsid += _serial;
	}

	~controller() {
		int ret;
		if (_claimed) {
			ret = libusb_release_interface(_handle, INTERFACE);
			log(ALERT, ret < 0) << "libusb_release_interface: " << usb_perror(ret) << endl;
		}

		if (_kernel_detached) {
			ret = libusb_attach_kernel_driver(_handle, INTERFACE);
			log(ALERT, ret < 0) << "libusb_attach_kernel_driver: " << usb_perror(ret) << endl;
		}

		libusb_close(_handle);
	}

	int claim() {
		int ret;
		if (libusb_kernel_driver_active(_handle, INTERFACE)) {
			ret = libusb_detach_kernel_driver(_handle, INTERFACE);
			if (ret < 0) {
				log(ALERT) << "libusb_detach_kernel_driver: " << usb_perror(ret) << endl;
				return ret;
			}
			_kernel_detached = true;
		}

		ret = libusb_claim_interface(_handle, INTERFACE);
		if (ret < 0) {
			log(ALERT) << "libusb_claim_interface: " << usb_perror(ret) << endl;
			return ret;
		}
		_claimed = true;
		return 0;
	}

	int get_data() {
		int ret = claim();
		if (ret)
			return ret;

		// get HID descriptor
		unsigned char buf[1024];
		ret = libusb_get_descriptor(_handle, LIBUSB_DT_HID, 0, buf, 255);
		if (ret < 0) {
			log(ALERT) << "hid-desc: " << usb_perror(ret) << endl;
		} else {
			usb_hid_descriptor *hid = (usb_hid_descriptor *)buf;
			log(INFO) << "HID: " << int(hid->bDescriptorType) << " / " << int(hid->bNumDescriptors) << endl;
			for (int n = 0; n < int(hid->bNumDescriptors); n++) {
				unsigned len = hid->wDescriptorLength(n);
				log(INFO) << "\t" << int(hid->descriptors[n].bDescriptorType) << " / " << len << endl;

				if (len < sizeof(buf)) {
					ret = libusb_get_descriptor(_handle, LIBUSB_DT_REPORT, 0, buf, len);
					if (ret >= 0) {
						log(INFO) << "\t\treq=" << len << "  rec=" << ret << endl << endl;
						parse_report(buf, ret);
					}
				}
			}
		}

		// get HID report descriptor
		int len;
		ret = libusb_interrupt_transfer(_handle, LIBUSB_ENDPOINT_IN|1, buf, sizeof(buf), &len, 100 /* ms */);
		log(ALERT, ret < 0) << "transfer: " << usb_perror(ret) << ", " << len << endl;

		for (int i = 0; i < len; i++)
			log(ALWAYS) << hex << setw(2) << setfill('0') << int(buf[i]) << "  ";
		log(ALWAYS) << dec << endl;

		return ret;
	}

	void parse_report(unsigned char *buf, int len) {
		int usage_table = 0; // "undefined"  (Hut1_11.pdf:15)
		int usage = 0; // "undefined"  (Hut1_11.pdf:27)
		string indent = "";
		for (unsigned char *b = buf; b < buf + len; ) {
			if (*b == 0xfe) { // long item
				int size = *++b;
				int tag = *++b;
				log(INFO) << "L s=" << size << " t=" << tag << endl;
				b += size;

			} else {          // short item
				int size = *b & 0x3;

				ostringstream x;
				x << setw(3) << int(b - buf) << ": \033[30;1m" << hex << setw(2) << setfill('0') << int(*b) << ' ';
				for (int i = 0; i < size; i++)
					x << hex << setw(2) << setfill('0') << int(b[i + 1]) << ' ';
				for (int i = size; i < 3; i++)
					x << "\033[m   ";
				string X = x.str();

				int type = (*b >> 2) & 0x3;
				int tag = (*b++ >> 4) & 0xf;

				int value = 0;
				if (size > 0)
					value = *b++;
				if (size > 1)
					value += *b++ << 8;
				if (size > 2)
					value += (*b++ << 16), value += (*b++ << 24);

				log(INFO) << X << "\t";
				if (type == 0) { // Main
					const char *s;
					string iof;
					switch (tag) {
					case 0x8: s = "Input"; iof = parse_iof(0, value); break;
					case 0x9: s = "Output"; iof = parse_iof(1, value); break;
					case 0xa: { // Collection
						switch (value) {
						case 0: s = "Physical"; break;
						case 1: s = "Application"; break;
						case 2: s = "Logical"; break;
						default: s = "bu0836a: not handled"; break;
						}
						log(INFO) << indent << "\033[35mCollection '" << s << "'\033[m" << endl;
						indent += "\t";
						continue;
					}
					case 0xb: s = "Feature"; iof = parse_iof(2, value); break;
					case 0xc: { // End Collection
						indent = indent.substr(0, indent.length() - 1);
						log(INFO) << indent << "\033[35mEnd Collection\033[m" << endl;
						continue;
					}
					default:  s = "Reserved"; break;
					}
					log(INFO) << indent << "\033[35m" << s << " '" << iof << "'\033[m" << endl;

				} else if (type == 1) { // Global
					const char *s;
					switch (tag) {
					case 0x0: { // Usage Page
						usage_table = value;
						switch (value) {
						case 0: s = "Undefined"; break;
						case 1: s = "Generic Desktop Controls"; break;
						case 9: s = "Button"; break;
						case 0xff00: s = "Vendor-defined"; break;
						default: s = "bu0836a: not handled"; break;
						}
						log(INFO) << indent << "\033[33mUsage Page '" << s << "'\033[m" << endl;
						continue;
					}
					case 0x1: s = "Logical Minimum"; break;
					case 0x2: s = "Logical Maximum"; break;
					case 0x3: s = "Physical Minimum"; break;
					case 0x4: s = "Physical Maximum"; break;
					case 0x5: s = "Unit Exponent"; break;
					case 0x6: s = "Unit"; break;
					case 0x7: s = "Report Size"; break;
					case 0x8: s = "Report ID"; break;
					case 0x9: s = "Report Count"; break;
					case 0xa: s = "Push"; break;
					case 0xb: s = "Pop"; break;
					default:  s = "Reserved"; break;
					}
					log(INFO) << indent << "\033[33m" << s << " = " << value << "\033[m" << endl;

				} else if (type == 2) { // Local
					const char *s;
					switch (tag) {
					case 0x0: { // Usage
						usage = value;
						if (usage_table == 1) {
							switch (value) {
								case 0x00: s = "Undefined"; break;
								case 0x01: s = "Pointer"; break;
								case 0x04: s = "Joystick"; break;
								case 0x30: s = "X"; break;
								case 0x31: s = "Y"; break;
								case 0x32: s = "Z"; break;
								case 0x33: s = "Rx"; break;
								case 0x34: s = "Ry"; break;
								case 0x35: s = "Rz"; break;
								case 0x36: s = "Slider"; break;
								case 0x37: s = "Dial"; break;
								case 0x38: s = "Wheel"; break;
								case 0x39: s = "Hat switch"; break;
								default: s = "bu0836a: not handled"; break;
							}
							log(INFO) << indent << "\033[36mUsage '" << s << "'\033[m" << endl;
							continue;
						}
						s = "Usage"; break;
					}
					case 0x1: s = "Usage Minimum"; break;
					case 0x2: s = "Usage Maximum"; break;
					case 0x3: s = "Designator Index"; break;
					case 0x4: s = "Designator Minimum"; break;
					case 0x5: s = "Designator Maximum"; break;
					case 0x7: s = "String Index"; break;
					case 0x8: s = "String Minimum"; break;
					case 0x9: s = "String Maximum"; break;
					case 0xa: s = "Delimiter"; break;
					case 0xb: s = "Pop"; break;
					default:  s = "Reserved"; break;
					}
					log(INFO) << indent << "\033[36m" << s << " = " << value << "\033[m" << endl;

				} else { // Reserved
					log(INFO) << "\033[1mReserved: value = " << value << "\033[m" << endl;
					//for (int i = 0; i < size; i++)
					//	log(INFO) << hex << setw(2) << setfill('0') << int(*b++) << ' ';
					//log(INFO) << dec << endl;
				}
			}
		}
		log(INFO) << endl;
	}

	string parse_iof(int mode, int value) {
		string s;
		s += value & 0x01 ? "*const " : "data ";
		s += value & 0x02 ? "*var " : "array ";
		s += value & 0x04 ? "*rel " : "abs ";
		s += value & 0x08 ? "*non-lin " : "lin ";
		s += value & 0x10 ? "*no-pref-state " : "pref-state ";
		s += value & 0x20 ? "*no-null-state " : "null-pos ";
		if (mode > 0)
			s += value & 0x40 ? "*vol " : "non-vol ";
		s += value & 0x80 ? "*buff-bytes" : "bit-field";
		return s;
	}

	const string &bus_address() const { return _bus_address; }
	const string &jsid() const { return _jsid; }
	const string &serial() const { return _serial; }
	void print_info() const { cout << _bus_address << "   \"" << _jsid << '"' << "   ver " << _release << endl; }

private:
	string _bus_address;
	string _id;
	string _release;
	string _manufacturer;
	string _product;
	string _serial;
	string _jsid;
	libusb_device_handle *_handle;
	libusb_device *_device;
	libusb_device_descriptor _desc;
	bool _claimed;
	bool _kernel_detached;

	static const int INTERFACE = 0;
};



class bu0836a {
public:
	bu0836a(int debug_level = 3) {
		int ret = libusb_init(0);
		if (ret)
			throw string("libusb_init: ") + usb_perror(ret);
		libusb_set_debug(0, 3);

		libusb_device **list;
		for (int i = 0; i < libusb_get_device_list(0, &list); i++) {
			libusb_device_handle *handle;
			int ret = libusb_open(list[i], &handle);

			if (ret) {
				log(ALERT) << "\terror: libusb_open: " << usb_perror(ret) << endl;
				continue;
			}

			libusb_device *dev = libusb_get_device(handle);

			libusb_device_descriptor desc;
			ret = libusb_get_device_descriptor(dev, &desc);
			if (ret)
				log(ALERT) << "error: libusb_get_device_descriptor: " << usb_perror(ret) << endl;
			else if (desc.idVendor == _vendor && desc.idProduct == _product)
				_devices.push_back(new controller(handle, dev, desc));
			else
				libusb_close(handle);
		}
		libusb_free_device_list(list, 1);
	}

	~bu0836a()
	{
		vector<controller *>::const_iterator it, end = _devices.end();
		for (it = _devices.begin(); it != end; ++it)
			delete *it;
		libusb_exit(0);
	}

	void print_list() const
	{
		vector<controller *>::const_iterator it, end = _devices.end();
		for (it = _devices.begin(); it != end; ++it)
			(*it)->print_info();
	}

	int find(string which, controller **ctrl) const
	{
		int num = 0;
		vector<controller *>::const_iterator it, end = _devices.end();
		for (it = _devices.begin(); it != end; ++it) {
			if ((*it)->serial().find(which) != string::npos) {
				*ctrl = *it;
				num++;
			}
		}
		if (num != 1)
			*ctrl = 0;
		return num;
	}

	vector<controller *> &devices() { return _devices; }

private:
	static const int _vendor = 0x16c0;
	static const int _product = 0x05ba;
	vector<controller *> _devices;
};



int main(int argc, const char *argv[])
try {
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

	init_option_parser(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE)
		if (option == HELP_OPTION)
			help();

	bu0836a usb;
	controller *selected = 0;

	int numdev  = usb.devices().size();
	if (numdev == 1)
		selected = usb.devices()[0];
	else if (!numdev)
		throw string("no BU0836A found");

	init_option_parser(&ctx, argc, argv, options);
	while ((option = get_option(&ctx)) != OPTIONS_DONE) {
		switch (option) {
		case VERBOSE_OPTION:
			config::verbosity++;
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
			usb.print_list();
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
