#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>

#include "bu0836.hxx"
#include "hid.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;

namespace {

#ifdef VALGRIND
bool interrupted = true;
#else
bool interrupted = false;
#endif



void interrupt_handler(int)
{
	log(INFO) << RED << "Interrupted" << NORM << endl;
	interrupted = true;
}



const char *usb_strerror(int errno)
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



string bcd2str(int n)
{
	ostringstream o;
	o << hex << ((n >> 12) & 0xf) << ((n >> 8) & 0xf) << '.' << ((n >> 4) & 0xf) << (n & 0xf);
	return o.str();
}



string strip(string s)
{
	const char spaces[] = " \t\n\r\b";
	string::size_type start = s.find_first_not_of(spaces, 0);
	string::size_type end = s.find_last_not_of(spaces, string::npos);
	return start == string::npos || end == string::npos ? "" : s.substr(start, end - start + 1);
}

} // namespace



controller::controller(libusb_device_handle *handle, libusb_device *device, libusb_device_descriptor desc) :
	_handle(handle),
	_device(device),
	_desc(desc),
	_hid_descriptor(0),
	_claimed(false),
	_kernel_detached(false)
{
	assert(sizeof(_eeprom) == 0x100); // FIXME + <cassert>

	ostringstream s;
	s << int(libusb_get_bus_number(_device)) << ':' << int(libusb_get_device_address(_device));
	_bus_address = s.str();

	s.str("");
	s << hex << setfill('0') << setw(4) << _desc.idVendor << ':' << setw(4) << _desc.idProduct;
	_id = s.str();

	_release = bcd2str(_desc.bcdDevice);

	unsigned char buf[256];
	if (_desc.iManufacturer && libusb_get_string_descriptor_ascii(_handle, _desc.iManufacturer, buf, sizeof(buf)) > 0)
		_manufacturer = strip(string((char *)buf));

	if (_desc.iProduct && libusb_get_string_descriptor_ascii(_handle, _desc.iProduct, buf, sizeof(buf)) > 0)
		_product = strip(string((char *)buf));

	if (_desc.iSerialNumber && libusb_get_string_descriptor_ascii(_handle, _desc.iSerialNumber, buf, sizeof(buf)) > 0)
		_serial = strip(string((char *)buf));

	_jsid = _manufacturer;
	if (!_manufacturer.empty() && !_product.empty())
		_jsid += ' ';
	_jsid += _product;
	if (!_jsid.empty() && !_serial.empty())
		_jsid += ' ';
	_jsid += _serial;
}



controller::~controller()
{
	int ret;
	if (_claimed) {
		ret = libusb_release_interface(_handle, _INTERFACE);
		if (ret < 0)
			log(ALERT) << "libusb_release_interface: " << usb_strerror(ret) << endl;
	}

	if (_kernel_detached) {
		ret = libusb_attach_kernel_driver(_handle, _INTERFACE);
		if (ret < 0)
			log(ALERT) << "libusb_attach_kernel_driver: " << usb_strerror(ret) << endl;
	}

	libusb_close(_handle);
	delete [] _hid_descriptor;
}



int controller::claim()
{
	int ret;
	if (!_kernel_detached && libusb_kernel_driver_active(_handle, _INTERFACE)) {
		ret = libusb_detach_kernel_driver(_handle, _INTERFACE);
		if (ret < 0) {
			log(ALERT) << "libusb_detach_kernel_driver: " << usb_strerror(ret) << endl;
			return ret;
		}
		_kernel_detached = true;
	}

	if (!_claimed) {
		ret = libusb_claim_interface(_handle, _INTERFACE);
		if (ret < 0) {
			log(ALERT) << "libusb_claim_interface: " << usb_strerror(ret) << endl;
			return ret;
		}
		_claimed = true;
		parse_hid();
	}

	return 0;
}



int controller::parse_hid()
{
	unsigned char buf[255];
	int ret = libusb_get_descriptor(_handle, LIBUSB_DT_HID, 0, buf, sizeof(buf));
	if (ret < 0) {
		log(ALERT) << "libusb_get_descriptor: " << usb_strerror(ret) << endl;
		return ret;
	}

	_hid_descriptor = (usb_hid_descriptor *)new unsigned char[ret];
	memcpy(_hid_descriptor, buf, ret);
	int numreports = _hid_descriptor->bNumDescriptors;

	log(BULK) << "HID: type=" << int(_hid_descriptor->bDescriptorType) << "  num=" << numreports << endl;

	for (int i = 0; i < numreports; i++) {
		int len = _hid_descriptor->wDescriptorLength(i);
		log(BULK) << "REPORT " << i << ": type=" << int(_hid_descriptor->descriptors[i].bDescriptorType)
				<< "  len=" << len << endl;

		unsigned char *buf = new unsigned char[len];
		ret = libusb_get_descriptor(_handle, LIBUSB_DT_REPORT, 0, buf, len);
		if (ret < 0) {
			log(ALERT) << "libusb_get_descriptor/LIBUSB_DT_REPORT: " << usb_strerror(ret) << endl;
		} else if (ret != len) {
			log(ALERT) << "libusb_get_descriptor/LIBUSB_DT_REPORT: only " << ret << " of " << len
					<< " bytes delivered" << endl;
		} else {
			_hid.parse(buf, ret);
		}
		delete [] buf;
	}
	return 0;
}



int controller::get_image()
{
	int ret = claim();
	if (ret)
		return ret;

	unsigned char buf[17];
	int progress = 0xffff;
	int maxtries = 50;
	while (progress && maxtries--) {
		ret = libusb_control_transfer(_handle, /* CLASS SPECIFIC REQUEST IN */ 0xa1, /* GET_REPORT */ 0x01,
				/* FEATURE */ 0x0300, 0, buf, sizeof(buf), 1000 /* ms */);
		if (ret < 0) {
			log(ALERT) << "get_image/libusb_control_transfer: " << usb_strerror(ret) << endl;
			return -1;
		}
		if (ret != sizeof(buf))
			continue;
		if (buf[0] & 0x0f)
			continue;
		progress &= ~(1 << (buf[0] >> 4));
		memcpy(reinterpret_cast<uint8_t *>(&_eeprom) + buf[0], buf + 1, 16);
	}
	if (!maxtries) {
		log(ALERT) << "get_image: maxtries" << endl;  // FIXME msg
		return -2;
	}
	return 0;
}



int controller::print_status()
{
	int ret = get_image();
	if (ret)
		return ret; // FIXME handle ret value

	cout << BBLACK << "_____________________________ Axes ____________________________" << NORM << endl << endl;
	cout << "            #0     #1     #2     #3     #4     #5     #6     #7" << endl;

	cout << "inverted:    ";
	for (int i = 0; i < 8; i++)
		if (get_invert(i))
			cout << RED << "I      ";
		else
			cout << GREEN << "-      ";
	cout << NORM << endl;

	cout << "zoom:  ";
	for (int i = 0; i < 8; i++) {
		int zoom = get_zoom(i);
		cout << (zoom ? RED : GREEN) << setw(7) << zoom;
	}
	cout << NORM << endl << endl << endl;

	const char *s[4] = { " -   -  ", "\\_1:1_/ ", "\\_1:2_/ ", "\\_1:4_/ " };
	cout << BBLACK << "_______________________ Buttons/Encoders ______________________" << NORM << endl << endl;
	cout << "#00 #01 #02 #03 #04 #05 #06 #07 #08 #09 #10 #11 #12 #13 #14 #15" << endl;
	for (int i = 0; i < 16; i += 2) {
		int m = get_encoder_mode(i);
		cout << (m ? RED : GREEN) << s[m];
	}
	cout << NORM << endl << endl;

	cout << "#16 #17 #18 #19 #20 #21 #22 #23 #24 #25 #26 #27 #28 #29 #30 #31" << endl;
	for (int i = 16; i < 32; i += 2) {
		int m = get_encoder_mode(i);
		cout << (m ? RED : GREEN) << s[m];
	}
	cout << NORM << endl << endl;

	cout << "pulse width: ";
	int pulse = get_pulse_width();
	if (pulse == 6) // 48 ms
		cout << GREEN;
	else
		cout << RED;
	cout << pulse * 8 << " ms" << NORM << endl << endl;
	return 0;
}



int controller::set_image(int which)
{
	int ret = claim();
	if (ret)
		return ret;

	unsigned char buf[17];
	for (unsigned char i = 0; i < 16; i++) {
		if (!(which & (1 << i)))
			continue;

		buf[0] = i << 4;
		memcpy(buf + 1, reinterpret_cast<uint8_t *>(&_eeprom) + buf[0], 16);
		ret = libusb_control_transfer(_handle, /* CLASS SPECIFIC REQUEST OUT */ 0x21, /* SET_REPORT */ 0x09,
				/* FEATURE */ 0x0300, 0, buf, sizeof(buf), 1000 /* ms */);
		if (ret < 0) {
			log(ALERT) << "set_image/libusb_control_transfer: " << usb_strerror(ret) << endl;
			return -1;
		}
	}
	return 0;
}



int controller::save_image(const char *path)
{
	ofstream file(path, ofstream::binary | ofstream::trunc);
	if (!file)
		throw string("cannot write to '") + path + '\'';
	file.write(reinterpret_cast<const char *>(&_eeprom), sizeof(_eeprom));

	file.seekp(0, ofstream::end);
	if (file.tellp() != sizeof(_eeprom))
		throw string("file '") + path + "' has wrong size";

	file.close();
	return 0;
}



int controller::load_image(const char *path)
{
	ifstream file(path, ifstream::binary);
	if (!file)
		throw string("cannot read from '") + path + '\'';
	file.read(reinterpret_cast<char *>(&_eeprom), sizeof(_eeprom));

	file.seekg(0, ifstream::end);
	if (file.tellg() != sizeof(_eeprom))
		throw string("file '") + path + "' has wrong size";

	file.close();
	return 0;
}



int controller::show_input_reports()
{
	int ret = claim();
	if (ret)
		return ret;
	if (_hid.data().empty()) {
		log(ALERT) << "show_input_reports: no hid data" << endl;
		return 1;
	}

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = interrupt_handler;
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	unsigned char buf[1024];
	do {
		int len;
		ret = libusb_interrupt_transfer(_handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &len, 100 /* ms */);
		if (ret < 0) {
			log(ALERT) << "transfer: " << usb_strerror(ret) << ", " << len << endl;
			sleep(2);
			continue;
		}

		log(BULK) << endl << bytes(buf, len) << endl;
		log(INFO) << endl;
		_hid.print_input_report(_hid.data()[0], buf);

		usleep(100000);
	} while (!interrupted);

	return 0;
}



int controller::dump_internal_data()
{
	int ret = claim();
	if (ret)
		return ret;

	// display EEPROM image
	if (get_image())
		throw string(ORIGIN);
	log(ALWAYS) << setfill('0') << hex;
	for (int i = 0; i < 16; i++)
		log(ALWAYS) << setw(2) << i * 16 << ' '
				<< bytes(reinterpret_cast<unsigned char *>(&_eeprom) + i * 16, 16) << endl;
	log(ALWAYS) << dec;
	return 0;
}



bu0836::bu0836(int debug_level)
{
	int ret = libusb_init(_CONTEXT);
	if (ret < 0)
		throw string("libusb_init: ") + usb_strerror(ret);
	libusb_set_debug(_CONTEXT, debug_level);

	libusb_device **list;
	ret = libusb_get_device_list(_CONTEXT, &list);
	if (ret < 0)
		throw string("libusb_get_device_list: ") + usb_strerror(ret);

	for (int i = 0; i < ret; i++) {
		libusb_device_handle *handle;
		int ret = libusb_open(list[i], &handle);

		if (ret) {
			log(ALERT) << "\terror: libusb_open: " << usb_strerror(ret) << endl;
			continue;
		}

		libusb_device *dev = libusb_get_device(handle);

		libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(dev, &desc);
		if (ret)
			log(ALERT) << "error: libusb_get_device_descriptor: " << usb_strerror(ret) << endl;
		else if (desc.idVendor != _VOTI)
			libusb_close(handle);
		else if (desc.idProduct == _BU0836 || desc.idProduct == _BU0836A)
			_devices.push_back(new controller(handle, dev, desc));
	}
	libusb_free_device_list(list, 1);

	_selected = size() == 1 ? _devices[0] : 0;
}



bu0836::~bu0836()
{
	vector<controller *>::const_iterator it, end = _devices.end();
	for (it = _devices.begin(); it != end; ++it)
		delete *it;
	libusb_exit(_CONTEXT);
}



int bu0836::select(string which)
{
	int num = 0;
	vector<controller *>::const_iterator it, end = _devices.end();
	for (it = _devices.begin(); it != end; ++it)
		if ((*it)->serial().rfind(which) == (*it)->serial().size() - which.size()
				|| (*it)->bus_address() == which)
			_selected = *it, num++;

	if (num != 1)
		_selected = 0;
	return num;
}
