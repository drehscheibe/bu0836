#ifndef _BU0836A_HXX_
#define _BU0836A_HXX_

#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <vector>

#include <libusb.h>



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
	int get_data();
	int get_image();
	int set_image();
	int save_image(const char *);
	int load_image(const char *);

	const std::string &bus_address() const { return _bus_address; }
	const std::string &jsid() const { return _jsid; }
	const std::string &serial() const { return _serial; }
	void print_info() const { std::cout << _bus_address
			<< "  " << _manufacturer
			<< ", " << _product
			<< ", " << _serial
			<< ", v" << _release << std::endl; }

private:
	std::string _bus_address;
	std::string _id;
	std::string _release;
	std::string _manufacturer;
	std::string _product;
	std::string _serial;
	std::string _jsid;

	libusb_device_handle *_handle;
	libusb_device *_device;
	libusb_device_descriptor _desc;
	bool _claimed;
	bool _kernel_detached;
	unsigned char _image[256];

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
