#include <iostream>
#include <libusb.h>

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
	libusb_context *ctx;
	int ret = libusb_init(&ctx);
	if (ret) {
		cout << "ret=" << ret << endl;
		return ret;
	}

	libusb_set_debug(ctx, 3);

	libusb_device **list;
	ssize_t num = libusb_get_device_list(ctx, &list);
	for (int i = 0; i < num; i++) {
		libusb_device_handle *handle;
		int ret = libusb_open(list[i], &handle);

		cout << "#" << i << " ret=" << ret;
		if (ret) {
			cout << endl;
			continue;
		}

		libusb_device *dev = libusb_get_device(handle);
 		cout << " dev=" << dev;

		libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(dev, &desc);
		if (!ret) {
			cout << "  " << desc.idVendor << " : " << desc.idProduct;
		}

		cout << endl;
		libusb_close(handle);
	}

	cout << "searching " << 0x06a3 << " : " << 0x0006 << endl;

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
	return 0;
}
