#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "logging.h"
#include "hid_parser.h"

using namespace std;


const char *collection_string(int id)
{
	if (id >= 0x80 && id <= 0xff)
		return "Vendor-defined";
	switch (id) {
	case 0: return "Physical (group of axes)";
	case 1: return "Application (mouse, keyboard)";
	case 2: return "Logical (interrelated data)";
	case 3: return "Report";
	case 4: return "Named Array";
	case 5: return "Usage Switch";
	case 6: return "Usage Modifier";
	default: return "Reserved";
	}
}



const char *usage_table_string(int id)
{
	if (id >= 0xff00 && id <= 0xffff)
		return "Vendor-defined";

	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Generic Desktop Controls";
	case 0x02: return "Simulation Controls";
	case 0x03: return "VR Controls";
	case 0x04: return "Sport Controls";
	case 0x05: return "Game Controls";
	case 0x06: return "Generic Device Controls";
	case 0x07: return "Keyboard/Keypad";
	case 0x08: return "LEDs";
	case 0x09: return "Button";
	case 0x0a: return "Ordinal";
	case 0x0b: return "Telephony";
	case 0x0c: return "Consumer";
	case 0x0d: return "Digitizer";
	case 0x0f: return "PID Page";
	case 0x10: return "Unicode";
	case 0x14: return "Alphanumeric Display";
	case 0x40: return "Medical Instruments";
	case 0x80: case 0x81: case 0x82: case 0x83: return "Monitor pages";
	case 0x84: case 0x85: case 0x86: case 0x87: return "Power pages";
	case 0x8c: return "Bar Code Scanner page";
	case 0x8d: return "Scale page";
	case 0x8e: return "Magnetic Stripe Reading (MSR) Devices";
	case 0x8f: return "Reserved Point of Sale pages";
	case 0x90: return "Camera Control Page";
	case 0x91: return "Arcade Page";
	default: return "Reserved";
	}
}



const char *generic_desktop_page_string(int id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Pointer";
	case 0x02: return "Mouse";
	case 0x04: return "Joystick";
	case 0x05: return "Gamepad";
	case 0x06: return "Keyboard";
	case 0x07: return "Keypad";
	case 0x08: return "Multi-axis Controller";
	case 0x30: return "X";
	case 0x31: return "Y";
	case 0x32: return "Z";
	case 0x33: return "Rx";
	case 0x34: return "Ry";
	case 0x35: return "Rz";
	case 0x36: return "Slider";
	case 0x37: return "Dial";
	case 0x38: return "Wheel";
	case 0x39: return "Hat switch";
	case 0x3a: return "Counted Buffer";
	case 0x3b: return "Byte Count";
	case 0x3c: return "Motion Wakeup";
	case 0x3d: return "Start";
	case 0x3e: return "Select";
	case 0x40: return "Vx";
	case 0x41: return "Vy";
	case 0x42: return "Vz";
	case 0x43: return "Vbrx";
	case 0x44: return "Vbry";
	case 0x45: return "Vbrz";
	case 0x46: return "Vno";
	case 0x47: return "Feature Notification";
	case 0x80: return "System Control";
	case 0x81: return "System Power Down";
	case 0x82: return "System Sleep";
	case 0x83: return "System Wake Up";
	case 0x84: return "System Context Menu";
	case 0x85: return "System Main Menu";
	case 0x86: return "System App Menu";
	case 0x87: return "System Menu Help";
	case 0x88: return "System Menu Exit";
	case 0x89: return "System Menu Select";
	case 0x8a: return "System Menu Right";
	case 0x8b: return "System Menu Left";
	case 0x8c: return "System Menu Up";
	case 0x8d: return "System Menu Down";
	case 0x8e: return "System Cold Restart";
	case 0x8f: return "System Warm Restart";
	case 0x90: return "D-pad Up";
	case 0x91: return "D-pad Down";
	case 0x92: return "D-pad Right";
	case 0x93: return "D-pad Left";
	case 0xa0: return "System Dock";
	case 0xa1: return "System Undock";
	case 0xa2: return "System Setup";
	case 0xa3: return "System Break";
	case 0xa4: return "System Debugger Break";
	case 0xa5: return "Application Break";
	case 0xa6: return "Application Debugger Break";
	case 0xa7: return "System Speaker Mute";
	case 0xa8: return "System Hibernate";
	case 0xb0: return "System Display Invert";
	case 0xb1: return "System Display Internal";
	case 0xb2: return "System Display External";
	case 0xb3: return "System Display Both";
	case 0xb4: return "System Display Dual";
	case 0xb5: return "System Display Toggle Int/Ext";
	case 0xb6: return "System Display Swap Primary/Secondary";
	case 0xb7: return "System Display LCD Autoscale";
	default: return "Reserved";
	}
}



string input_output_feature_string(int mode, int value) {
	string s;
	s += value & 0x01 ? "*const " : "data ";
	s += value & 0x02 ? "*var " : "array ";
	s += value & 0x04 ? "*rel " : "abs ";
	s += value & 0x08 ? "*non-lin " : "lin ";
	s += value & 0x10 ? "*no-pref-state " : "pref-state ";
	s += value & 0x20 ? "*no-null-state " : "null-pos ";
	if (mode > 0)
		s += value & 0x40 ? "*vol " : "non-vol ";
	s += value & 0x80 ? "*buff-bytes" : "bit-field";
	return s;
}



string unit_string(uint32_t u)
{
	string s;
	s += "System=";
	switch (u & 0xf) {         // nibble 0
	case 1: s += "SI-Linear"; break;
	case 2: s += "SI-Rotation"; break;
	case 3: s += "English-Linear"; break;
	case 4: s += "English-Rotation"; break;
	default: s += "None"; break;
	}
	s += " Length=";
	switch ((u >> 4) & 0xf) {  // nibble 1
	case 1: s += "Centimeter"; break;
	case 2: s += "Radians"; break;
	case 3: s += "Inch"; break;
	case 4: s += "Degrees"; break;
	default: s+= "None"; break;
	}
	s += " Mass=";
	switch ((u >> 8) & 0xf) {  // nibble 2
	case 1: case 2: s += "Gram"; break;
	case 3: case 4: s += "Slug"; break;
	default: s += "None"; break;
	}
	s += " Time=";
	switch ((u >> 12) & 0xf) { // nibble 3
	case 0: s += "None"; break;
	default: s += "Seconds"; break;
	}
	s += " Temperature=";
	switch ((u >> 16) & 0xf) { // nibble 4
	case 1: case 2: s += "Kelvin"; break;
	case 3: case 4: s += "Fahrenheit"; break;
	default: s += "None"; break;
	}
	s += " Current=";
	switch ((u >> 20) & 0xf) { // nibble 5
	case 0: s += "None"; break;
	default: s += "Ampere"; break;
	}
	s += " Luminous-intensity=";
	switch ((u >> 24) & 0xf) { // nibble 6
	case 0: s += "None"; break;
	default: s += "Candela"; break;
	}
	if ((u >> 28) & 0xf)
		log(WARN) << "use of reserved unit nibble 7" << endl;
	return s;
}



static string hexstr(const unsigned char *p, int num, int width)
{
	ostringstream x;
	x << hex << setfill('0');
	for (int i = 0; i < num; i++)
		x << setw(2) << int(p[i]) << ' ';
	string s = x.str();
	x.str("");
	x << left << setw(width) << setfill(' ') << s;
	return x.str();
}



hid_parser::hid_parser()
{
	_stack.push_back(hid_report_data());
	_global = &_stack[0];
}



void hid_parser::parse(const unsigned char *data, int len)
{
	for (const unsigned char *d = data; d < data + len; ) {
		if (*d == 0xfe) { // long item
			int size = *++d;
			int tag = *++d;

			log(ALERT) << ORIGIN"skipping unsupported long item" << endl;
			d += size;

		} else {          // short item
			int size = *d & 0x3;
			if (size == 3)
				size++;

			log(INFO) << dec << setw(3) << d - data << ": \033[30;1m" << hexstr(d, 1 + size, 19) << "\033[m";

			int type = (*d >> 2) & 0x3;
			int tag = (*d++ >> 4) & 0xf;
			uint32_t value = 0;
			if (size > 0)
				value = *d++;
			if (size > 1)
				value += *d++ << 8;
			if (size > 2)
				value += *d++ << 16, value += *d++ << 24;

			if (type == 0) {        // Main
				log(INFO) << "\033[35m";
				do_main(tag, value);
				log(INFO) << "\033[m" << endl;

			} else if (type == 1) { // Global
				log(INFO) << _indent << "\033[33m";
				do_global(tag, value);
				log(INFO) << "\033[m" << endl;

			} else if (type == 2) { // Local
				log(INFO) << _indent << "\033[36m";
				do_local(tag, value);
				log(INFO) << "\033[m" << endl;

			} else {                // Reserved
				log(INFO) << _indent << "Reserved" << endl;
				log(ALERT) << ORIGIN"short item: skipping item of reserved type" << endl;
			}
		}
	}
}


void hid_parser::do_main(int tag, uint32_t value)
{
	switch (tag) {
	case 0x8: // Input
		log(INFO) << _indent << "Input " << input_output_feature_string(0, value);
		break;
	case 0x9: // Output
		log(INFO) << _indent << "Output " << input_output_feature_string(1, value);
		break;
	case 0xa: // Collection
		log(INFO) << _indent << "Collection '" << collection_string(value) << '\'';
		_indent += '\t';
		break;
	case 0xb: // Feature
		log(INFO) << _indent << "Feature " << input_output_feature_string(2, value);
		break;
	case 0xc: // End Collection
		_indent = _indent.substr(0, _indent.length() - 1);
		log(INFO) << _indent << "End Collection ";
		break;
	}
}


void hid_parser::do_global(int tag, uint32_t value)
{
	switch (tag) {
	case 0x0:
		log(INFO) << "Usage Page '" << usage_table_string(value) << '\'';
		_global->usage_table = value;
		return;
	case 0x1:
		log(INFO) << "Logical Minimum = " << value;
		_global->logical_minimum = value;
		return;
	case 0x2:
		log(INFO) << "Logical Maximum = " << value;
		_global->logical_maximum = value;
		return;
	case 0x3:
		log(INFO) << "Physical Minimum = " << value;
		_global->physical_minimum = value;
		return;
	case 0x4:
		log(INFO) << "Physical Maximum = " << value;
		_global->physical_maximum = value;
		return;
	case 0x5:
		log(INFO) << "Unit Exponent = " << value;
		_global->unit_exponent = value;
		return;
	case 0x6:
		log(INFO) << "Unit = " << unit_string(value);
		_global->unit = value;
		return;
	case 0x7:
		log(INFO) << "Report Size = " << value;
		_global->report_size = value;
		return;
	case 0x8:
		log(INFO) << "Report ID = " << value;
		_global->report_id = value;
		return;
	case 0x9:
		log(INFO) << "Report Count = " << value;
		_global->report_count = value;
		return;
	case 0xa:
		log(INFO) << "Push";
		_stack.push_back(hid_report_data(_global));
		_global = &_stack[_stack.size() - 1];
		return;
	case 0xb:
		log(INFO) << "Pop";
		if (_stack.size() > 1) {
			_stack.pop_back();
			_global = &_stack[_stack.size() - 1];
		} else {
			log(ALERT) << ORIGIN"can't pop -- stack empty" << endl;
		}
		return;
	default:
		log(WARN) << ORIGIN"skipping reserved global item" << endl;
	}
}



void hid_parser::do_local(int tag, uint32_t value)
{
	switch (tag) {
	case 0x0: {
		log(INFO) << "Usage '";
		switch (_global->usage_table) {
		case 1:
			log(INFO) << generic_desktop_page_string(value);
			break;;
		default:
			log(INFO) << value;
		}
		log(INFO) << '\'';
		return;
	}
	case 0x1: log(INFO) << "Usage Minimum"; break;
	case 0x2: log(INFO) << "Usage Maximum"; break;
	case 0x3: log(INFO) << "Designator Index"; break;
	case 0x4: log(INFO) << "Designator Minimum"; break;
	case 0x5: log(INFO) << "Designator Maximum"; break;
	case 0x6: log(INFO) << "???"; break;
	case 0x7: log(INFO) << "String Index"; break;
	case 0x8: log(INFO) << "String Minimum"; break;
	case 0x9: log(INFO) << "String Maximum"; break;
	case 0xa: log(INFO) << "Delimiter"; break;
	default: log(INFO) << "Reserved"; break;
	}
	log(INFO) << " = " << value;
}
