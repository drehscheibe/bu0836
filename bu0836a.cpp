#include <cstdio>
#include <iostream>
#include <libusb.h>
#include <vector>

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


class bu0836a {
public:
	bu0836a(int debug_level = 3) {
		int ret = libusb_init(0);
		if (ret)
			throw;
		libusb_set_debug(0, 3);

		libusb_device **list;
		for (int i = 0; i < libusb_get_device_list(0, &list); i++) {
			cerr << "#" << i << ":" << endl;
			libusb_device_handle *handle;
			int ret = libusb_open(list[i], &handle);

			if (ret) {
				cerr << "\terror: libusb_open: " << usb_perror(ret) << endl;
				continue;
			}

			libusb_device *dev = libusb_get_device(handle);

			libusb_device_descriptor desc;
			ret = libusb_get_device_descriptor(dev, &desc);
			if (ret) {
				cerr << "error: libusb_get_device_descriptor: " << usb_perror(ret) << endl;
			} else {
				char release[6];
				BCD2STR(release, desc.bcdDevice);
				cerr
						<< "\t" << 0+libusb_get_bus_number(dev) << ":" << 0+libusb_get_device_address(dev)
						<< "\tvendor = " << desc.idVendor
						<< "\tproduct = " << desc.idProduct
						<< "\tversion = " << release
						<< "\tserial = " << 0 + desc.iSerialNumber
						<< endl;

				unsigned char buf[256];
#define STRING(x) libusb_get_string_descriptor_ascii(handle, desc.i##x, buf, sizeof(buf))
				if (desc.iManufacturer && STRING(Manufacturer) > 0)
					cerr << "\tmanufacturer = " << buf << endl;

				if (desc.iProduct && STRING(Product) > 0)
					cerr << "\tproduct = " << buf << endl;

				if (desc.iSerialNumber && STRING(SerialNumber) > 0)
					cerr << "\tserial number = " << buf << endl;
#undef STRING
				if (desc.idVendor == _vendor && desc.idProduct == _product) {
					cerr << "\t\tFOUND" << endl;
					_devices.push_back(list[i]);
				}
			}

			libusb_close(handle);
		}
		libusb_free_device_list(list, 1);
	}

	~bu0836a()
	{
		libusb_exit(0);
	}

private:
	static const int _vendor = 0x1130;
	static const int _product = 0xf211;
	vector<libusb_device *> _devices;
};


int main(int argc, char *argv[])
{
	bu0836a usb;
	return 0;
}
