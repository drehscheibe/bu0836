#ifndef _BU0836A_HXX_
#define _BU0836A_HXX_

#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <vector>

#include <libusb.h>

#include "hid.hxx"



struct usb_hid_descriptor {
	uint8_t  bLength;		// 9
	uint8_t  bDescriptorType;	// 33 -> LIBUSB_DT_HID
	uint16_t bcdHID;		// 1.10
	uint8_t  bCountryCode;		// 0 (unsupported)
	uint8_t  bNumDescriptors;	// 1
	struct {
		uint8_t bDescriptorType;	// 34 -> LIBUSB_DT_REPORT
		uint8_t wDescriptorLength1;	// 103
		uint8_t wDescriptorLength2;	// 103
	} descriptors[];

	inline int wDescriptorLength(int n) const {
		return libusb_le16_to_cpu(descriptors[n].wDescriptorLength2 << 8 | descriptors[n].wDescriptorLength1);
	}
};



class controller {
public:
	controller(libusb_device_handle *handle, libusb_device *device, libusb_device_descriptor desc);
	~controller();
	int claim();
	int get_image();
	int print_status();
	int set_image(int which = ~0);
	int save_image(const char *);
	int load_image(const char *);
	int show_input_reports();
	int dump_internal_data();

	const std::string &bus_address() const { return _bus_address; }
	const std::string &id() const { return _id; }
	const std::string &manufacturer() const { return _manufacturer; }
	const std::string &product() const { return _product; }
	const std::string &serial() const { return _serial; }
	const std::string &release() const { return _release; }
	const std::string &jsid() const { return _jsid; }

private:
	int parse_hid(void);
	int getrotmode (int b) const {
		uint16_t b0 = ((_eeprom.rotenc0[0] | (_eeprom.rotenc0[1] << 8)) >> b / 2) & 1;
		uint16_t b1 = ((_eeprom.rotenc1[0] | (_eeprom.rotenc1[1] << 8)) >> b / 2) & 1;
		return b0 | (b1 << 1);
	}
	void setrotmode(int b, int mode) {
		uint16_t b0 = _eeprom.rotenc0[0] | (_eeprom.rotenc0[1] << 8);
		uint16_t b1 = _eeprom.rotenc1[0] | (_eeprom.rotenc1[1] << 8);
		uint16_t mask = 1 << b / 2;
		b0 = mode & 1 ? b0 | mask : b0 & ~mask;
		b1 = mode & 2 ? b1 | mask : b1 & ~mask;
		_eeprom.rotenc0[0] = b0 & 0xff, _eeprom.rotenc0[1] = (b0 >> 8) & 0xff;
		_eeprom.rotenc1[0] = b1 & 0xff, _eeprom.rotenc1[1] = (b1 >> 8) & 0xff;
	}

	hid::hid _hid;

	std::string _bus_address;
	std::string _id;
	std::string _manufacturer;
	std::string _product;
	std::string _serial;
	std::string _release;
	std::string _jsid;

	libusb_device_handle *_handle;
	libusb_device *_device;
	libusb_device_descriptor _desc;
	usb_hid_descriptor *_hid_descriptor;
	bool _claimed;
	bool _kernel_detached;

	struct {
		uint8_t ___a[11];
		uint8_t invert;
		uint8_t ___b[2];
		uint8_t zoom[8];
		uint8_t rotenc0[2];
		uint8_t rotenc1[2];
		uint8_t pulse;
		uint8_t ___c[229];
	} _eeprom;

	static const int _INTERFACE = 0;
};



class bu0836 {
public:
	bu0836(int debug_level = 3);
	~bu0836();
	int find(std::string which, controller **ctrl) const;
	size_t size() const { return _devices.size(); }
	controller& operator[](unsigned int index) { return *_devices[index]; }

private:
	std::vector<controller *> _devices;

	static const int _CONTEXT = 0;
	static const int _VOTI = 0x16c0; // BODNAR products: 0x05b4--0x05bd, 0x2774--0x27d7
	static const int _BU0836 = 0x05b5;
	static const int _BU0836A = 0x05ba;
};

#endif
