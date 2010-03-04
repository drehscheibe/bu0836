#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <libusb.h>
#include "options.h"

using namespace std;


#define BCD2STR(s, n) snprintf(s, 6, "%x%x:%x%x", (n >> 12) & 0xf, (n >> 8) & 0xf, (n >> 4) & 0xf, n & 0xf)


static const char *usb_perror(int errno)
{
	switch (errno) {
	case LIBUSB_SUCCESS: return "success";
	case LIBUSB_ERROR_IO: return "input/output error";
	case LIBUSB_ERROR_INVALID_PARAM: return "invalid parameter";
	case LIBUSB_ERROR_ACCESS: return "access denied (insufficient permissions)";
	case LIBUSB_ERROR_NO_DEVICE: return "no such device (it may have been disconnected)";
	case LIBUSB_ERROR_NOT_FOUND: return "entity not found";
	case LIBUSB_ERROR_BUSY: return "resource busy";
	case LIBUSB_ERROR_TIMEOUT: return "operation timed out";
	case LIBUSB_ERROR_OVERFLOW: return "overflow";
	case LIBUSB_ERROR_PIPE: return "pipe error";
	case LIBUSB_ERROR_INTERRUPTED: return "system call interrupted (perhaps due to signal)";
	case LIBUSB_ERROR_NO_MEM: return "insufficient memory";
	case LIBUSB_ERROR_NOT_SUPPORTED: return "operation not supported or unimplemented on this platform";
	case LIBUSB_ERROR_OTHER: return "other error";
	default: return "unknown error code";
	}
}


static string strip(string s)
{
	const char space[] = " \t\n\r\b";
	string::size_type start = s.find_first_not_of(space, 0);
	string::size_type end = s.find_last_not_of(space, string::npos);
	return start == string::npos || end == string::npos ? "" : s.substr(start, end - start + 1);
}


class controller {
public:
	controller(libusb_device_handle *handle, libusb_device *device, libusb_device_descriptor desc) :
		_handle(handle),
		_device(device),
		_desc(desc)
	{
		ostringstream s;
		s << int(libusb_get_bus_number(_device)) << ":" << int(libusb_get_device_address(_device));
		_bus_address = s.str();

		s.str("");
		s << hex << setw(4) << setfill('0') << _desc.idVendor << ':' << setw(4) << setfill('0') << _desc.idProduct;
		_id = s.str();

		char release[6];
		BCD2STR(release, desc.bcdDevice);
		_release = string(release);

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
		print_info();
	}

	~controller() {
		libusb_close(_handle);
	}

	void print_info() const {
		cout << _bus_address
				<< "  " << _id
				<< "  version = '" << _release << '\''
				<< "  manu = '" << _manufacturer << '\''
				<< "  prod = '" << _product << '\''
				<< "  serial = '" << _serial << '\''
				<< "  jsid = '" << _jsid << '\''
				<< endl;
	}

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
};


class bu0836a {
public:
	bu0836a(int debug_level = 3) {
		int ret = libusb_init(0);
		if (ret)
			throw;
		libusb_set_debug(0, 3);

		libusb_device **list;
		for (int i = 0; i < libusb_get_device_list(0, &list); i++) {
			libusb_device_handle *handle;
			int ret = libusb_open(list[i], &handle);

			if (ret) {
				cerr << "\terror: libusb_open: " << usb_perror(ret) << endl;
				continue;
			}

			libusb_device *dev = libusb_get_device(handle);

			libusb_device_descriptor desc;
			ret = libusb_get_device_descriptor(dev, &desc);
			if (ret)
				cerr << "error: libusb_get_device_descriptor: " << usb_perror(ret) << endl;
			else if (libusb_cpu_to_le16(desc.idVendor) == _vendor && libusb_cpu_to_le16(desc.idProduct) == _product)
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

	vector<controller *> &devices() { return _devices; }

private:
	static const int _vendor = 0x16c0;
	static const int _product = 0x05ba;
	vector<controller *> _devices;
};


struct Options options[] = {
	{ "--help", "-h", 0 },
	{ "--verbose", "-v", 0 },
	{ "--device", "-d", 1 },
	{ "--list", "-l", 0 },
	{ "--normal", "-n", 1 },
	{ "--invert", "-i", 1 },
	{ "--button", "-b", 1 },
	{ "--rotary", "-r", 1 },
	{ 0, 0, 0 },
};

enum { HELP_OPT, VERBOSE_OPT, DEVICE_OPT, LIST_OPT, NORMAL_OPT, INVERT_OPT, BUTTON_OPT, ROTARY_OPT };


int option_handler(int index, const char *arg)
{
	switch (index) {
	case HELP_OPT:
		cerr << "Usage:  bu0836a [-n <number>] [-i <number>] ..." << endl;
		exit(0);
		break;
	case VERBOSE_OPT: cerr << "Verbose!" << endl; break;
	case DEVICE_OPT: cerr << "select device " << arg << endl; break;
	case LIST_OPT: cerr << "show device list" << endl; break;
	case NORMAL_OPT: cerr << "set axis " << arg << " to normal" << endl; break;
	case INVERT_OPT: cerr << "set axis " << arg << " to inverted" << endl; break;
	case BUTTON_OPT: cerr << "set up button " << arg << " for button function" << endl; break;
	case ROTARY_OPT: cerr << "set up button " << arg << " for rotary switch" << endl; break;

	case OPTIONS_TERMINATOR: break;
	case OPTIONS_ARGUMENT: cerr << "\033[33;1mARG: " << arg << "\033[m" << endl; break;

	case OPTIONS_EXCESS_ARGUMENT: cerr << "Ignoring assignment " << arg << endl; break;
	case OPTIONS_UNKNOWN_OPTION: cerr << "Unknown Option " << arg << endl; return OPTIONS_ABORT;
	case OPTIONS_MISSING_ARGUMENT: cerr << "Missing arg for " << arg << endl; return OPTIONS_ABORT;
	default: cerr << "\033[31;1mThis can't happen: " << index << "/" << arg << "\033[m" << endl; return OPTIONS_ABORT;
	}
	return OPTIONS_CONTINUE;
}


int main(int argc, const char *argv[])
{
	bu0836a usb;
	int next = parse_options(argc, argv, options, option_handler);

	for (int i = next; i < argc; i++)
		cerr << "\033[32mARG: " << argv[i] << "\033[m" << endl;

	return 0;
}
