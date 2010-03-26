#include <cstdlib>
#include <iostream>
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
	void print_info() const { std::cout << _bus_address << "   \"" << _jsid << '"' << "   ver " << _release << std::endl; }

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

	static const int INTERFACE = 0;
};



class bu0836a {
public:
	bu0836a(int debug_level = 3);
	~bu0836a();
	int find(std::string which, controller **ctrl) const;
	size_t size() const { return _devices.size(); }
	controller& operator[](unsigned int index) { return *_devices[index]; }

private:
	static const int _bodnar_id = 0x16c0;
	static const int _bu0836a_id = 0x05ba;
	std::vector<controller *> _devices;

	static const int CONTEXT = 0;
};
