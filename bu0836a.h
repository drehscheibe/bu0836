#include <iostream>

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)


struct usb_hid_descriptor {
	uint8_t  bLength;		// 9
	uint8_t  bDescriptorType;	// 33 -> LIBUSB_DT_HID
	uint16_t bcdHID;		// 1.10
	uint8_t  bCountryCode;		// 0 (unsupported)
	uint8_t  bNumDescriptors;	// 1
	struct {
		uint8_t  bDescriptorType;	// 34 -> LIBUSB_DT_REPORT
		uint16_t wDescriptorLength;	// 103
	} descriptors[];
};


std::ostream& log(int log_level, bool condition);
