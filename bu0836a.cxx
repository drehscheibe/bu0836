#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "bu0836a.hxx"
#include "hid_parser.hxx"
#include "logging.hxx"
#include "options.h"

using namespace std;



static const char *usb_strerror(int errno)
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



controller::controller(libusb_device_handle *handle, libusb_device *device, libusb_device_descriptor desc) :
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
		_jsid += " ";
	_jsid += _product;
	if (!_jsid.empty() && !_serial.empty())
		_jsid += " ";
	_jsid += _serial;
}



controller::~controller()
{
	int ret;
	if (_claimed) {
		ret = libusb_release_interface(_handle, INTERFACE);
		if (ret < 0)
			log(ALERT) << "libusb_release_interface: " << usb_strerror(ret) << endl;
	}

	if (_kernel_detached) {
		ret = libusb_attach_kernel_driver(_handle, INTERFACE);
		if (ret < 0)
			log(ALERT) << "libusb_attach_kernel_driver: " << usb_strerror(ret) << endl;
	}

	libusb_close(_handle);
}



int controller::claim()
{
	int ret;
	if (libusb_kernel_driver_active(_handle, INTERFACE)) {
		ret = libusb_detach_kernel_driver(_handle, INTERFACE);
		if (ret < 0) {
			log(ALERT) << "libusb_detach_kernel_driver: " << usb_strerror(ret) << endl;
			return ret;
		}
		_kernel_detached = true;
	}

	ret = libusb_claim_interface(_handle, INTERFACE);
	if (ret < 0) {
		log(ALERT) << "libusb_claim_interface: " << usb_strerror(ret) << endl;
		return ret;
	}
	_claimed = true;
	return 0;
}



int controller::get_data()
{
	int ret = claim();
	if (ret)
		return ret;

	hid_parser parser;

	// get HID descriptor
	unsigned char buf[1024];
	ret = libusb_get_descriptor(_handle, LIBUSB_DT_HID, 0, buf, 255);
	if (ret < 0) {
		log(ALERT) << "hid-desc: " << usb_strerror(ret) << endl;
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
					parser.parse(buf, ret);
					log(INFO) << endl;
				}
			}
		}
	}

	// read control pipe
	bool skip = true;
	for (int i = 0; i < 32; i++) {
		bzero(buf, 17);
		ret = libusb_control_transfer(_handle, /* CLASS SPECIFIC REQUEST IN */ 0xa1, /* GET_REPORT */ 0x01,
				/* FEATURE */ 0x0300, 0, buf, 17, 1000 /* ms */);
		if (skip && buf[0])
			continue;
		skip = false;
		if (ret == 17)
			log(INFO) << hexstr(buf, ret) << endl;
		else
			log(INFO) << i << "  control transfer: " << usb_strerror(ret) << "  (" << ret << ")" << endl;
		if (buf[0] == 0xf0)
			break;
	}

	// get HID report descriptor
	int len;
	ret = libusb_interrupt_transfer(_handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &len, 100 /* ms */);
	if (ret < 0)
		log(ALERT) << "transfer: " << usb_strerror(ret) << ", " << len << endl;

	uint16_t x = libusb_le16_to_cpu(*(uint16_t *)&buf[0]);
	uint16_t y = libusb_le16_to_cpu(*(uint16_t *)&buf[2]);
	log(INFO) << endl << hexstr(buf, len) << "  " << dec << "  x=" << x << "  y=" << y << endl;

	return ret;
}



bu0836a::bu0836a(int debug_level)
{
	int ret = libusb_init(0);
	if (ret < 0)
		throw string("libusb_init: ") + usb_strerror(ret);
	libusb_set_debug(0, debug_level);

	libusb_device **list;
	ret = libusb_get_device_list(0, &list);
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
		else if (desc.idVendor == _bodnar_id && desc.idProduct == _bu0836a_id)
			_devices.push_back(new controller(handle, dev, desc));
		else
			libusb_close(handle);
	}
	libusb_free_device_list(list, 1);
}



bu0836a::~bu0836a()
{
	vector<controller *>::const_iterator it, end = _devices.end();
	for (it = _devices.begin(); it != end; ++it)
		delete *it;
	libusb_exit(0);
}



int bu0836a::find(string which, controller **ctrl) const
{
	int num = 0;
	vector<controller *>::const_iterator it, end = _devices.end();
	for (it = _devices.begin(); it != end; ++it)
		if ((*it)->serial().find(which) != string::npos)
			*ctrl = *it, num++;

	if (num != 1)
		*ctrl = 0;
	return num;
}
