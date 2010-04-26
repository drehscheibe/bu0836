// bu0836 device manager
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
using namespace logging;



namespace bu0836 {

namespace {

#ifdef VALGRIND
bool interrupted = true;
#else
bool interrupted = false;
#endif



void interrupt_handler(int)
{
	log(INFO) << "Interrupted" << endl;
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



string strip(const string &s)
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
	_kernel_detached(false),
	_dirty(false)
{
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
	if (!_jsid.empty() && !_product.empty())
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
		ret = parse_hid();
		if (ret)
			return ret;

		ret = get_eeprom();
		if (ret)
			return ret;
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



int controller::get_eeprom()
{
	unsigned char buf[17];
	int progress = 0xffff;
	int maxtries = 50;
	while (progress && maxtries--) {
		int ret = libusb_control_transfer(_handle, /* CLASS SPECIFIC REQUEST IN */ 0xa1, /* GET_REPORT */ 0x01,
				/* FEATURE */ 0x0300, 0, buf, sizeof(buf), 1000 /* ms */);
		if (ret < 0) {
			log(ALERT) << "get_eeprom/libusb_control_transfer: " << usb_strerror(ret) << endl;
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
		log(ALERT) << "get_eeprom: maxtries" << endl;  // FIXME msg
		return -2;
	}
	return 0;
}



int controller::set_eeprom(unsigned int from, unsigned int to)
{
	unsigned char buf[2];
	unsigned char *e = reinterpret_cast<uint8_t *>(&_eeprom);
	for (unsigned char i = from; i <= to; i++) {
		buf[0] = i;
		buf[1] = e[i];
		//cerr << magenta << bytes(buf, 2) << reset << endl; // FIXME
		int ret = libusb_control_transfer(_handle, /* CLASS SPECIFIC REQUEST OUT */ 0x21, /* SET_REPORT */ 0x09,
				/* FEATURE */ 0x0300, 0, buf, 2, 1000 /* ms */);
		if (ret < 0) {
			log(ALERT) << "set_eeprom/libusb_control_transfer: " << usb_strerror(ret) << endl;
			return -1;
		}
	}
	return 0;
}



int controller::save_image_file(const char *path)
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



int controller::load_image_file(const char *path)
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
		int ret = libusb_interrupt_transfer(_handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &len, 100 /* ms */);
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



manager::manager(int debug_level)
{
	int ret = libusb_init(_CONTEXT);
	if (ret < 0)
		throw string("libusb_init: ") + usb_strerror(ret);
	libusb_set_debug(_CONTEXT, debug_level);

	libusb_device **list;
	int num = libusb_get_device_list(_CONTEXT, &list);
	if (num < 0)
		throw string("libusb_get_device_list: ") + usb_strerror(ret);

	for (int i = 0; i < num; i++) {
		libusb_device_handle *handle;
		ret = libusb_open(list[i], &handle);
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



manager::~manager()
{
	vector<controller *>::const_iterator it, end = _devices.end();
	for (it = _devices.begin(); it != end; ++it)
		delete *it;
	libusb_exit(_CONTEXT);
}



int manager::select(string which)
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

} // namespace bu0836
