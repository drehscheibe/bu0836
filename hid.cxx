#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "logging.hxx"
#include "hid.hxx"

using namespace std;



static string string_join(const vector<string> &v, const char *join = " ")
{
	size_t size = v.size();
	string s;
	if (size) {
		vector<string>::const_iterator it = v.begin();
		for (s = *it++; --size; ) {
			s += join;
			s += *it++;
		}
	}
	return s;
}



string hexstr(const unsigned char *p, unsigned int num, size_t width)
{
	ostringstream x;
	x << hex << setfill('0');
	while (num--)
		x << setw(2) << int(*p++) << ' ';
	string s = x.str();
	if (width > s.length())
		s.resize(width, ' ');
	return s;
}



const char *collection_string(uint32_t id)
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



const char *usage_table_string(uint32_t id) // 0x00
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



const char *generic_desktop_page_string(uint32_t id) // 0x01
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



const char *simulation_controls_page_string(uint32_t id) // 0x02
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Flight Simulation Device";
	case 0x02: return "Automobile Simulation Device";
	case 0x03: return "Tank Simulation Device";
	case 0x04: return "Spaceship Simulation Device";
	case 0x05: return "Submarine Simulation Device";
	case 0x06: return "Sailing Simulation Device";
	case 0x07: return "Motorcycle Simulation Device";
	case 0x08: return "Sports Simulation Device";
	case 0x09: return "Airplane Simulation Device";
	case 0x0a: return "Helicopter Simulation Device";
	case 0x0b: return "Magic Carpet Simulation Device";
	case 0x0c: return "Bicycle Simulation Device";
	case 0x20: return "Flight Control Stick";
	case 0x21: return "Flight Stick";
	case 0x22: return "Cyclic Control";
	case 0x23: return "Cyclic Trim";
	case 0x24: return "Flight Yoke";
	case 0x25: return "Track Control";
	case 0xb0: return "Aileron";
	case 0xb1: return "Aileron Trim";
	case 0xb2: return "Anti-Torque Control";
	case 0xb3: return "Autopilot Enable";
	case 0xb4: return "Chaff Release";
	case 0xb5: return "Collective Control";
	case 0xb6: return "Dive Break";
	case 0xb7: return "Electronic Countermeasures";
	case 0xb8: return "Elevator";
	case 0xb9: return "Elevator Trim";
	case 0xba: return "Rudder";
	case 0xbb: return "Throttle";
	case 0xbc: return "Flight Communications";
	case 0xbd: return "Flare Release";
	case 0xbe: return "Landing Gear";
	case 0xbf: return "Toe Break";
	case 0xc0: return "Trigger";
	case 0xc1: return "Weapons Arm";
	case 0xc2: return "Weapons Select";
	case 0xc3: return "Wing Flaps";
	case 0xc4: return "Accelerator";
	case 0xc5: return "Brake";
	case 0xc6: return "Clutch";
	case 0xc7: return "Shifter";
	case 0xc8: return "Steering";
	case 0xc9: return "Turret Direction";
	case 0xca: return "Barrel Elevation";
	case 0xcb: return "Dive Plane";
	case 0xcc: return "Ballast";
	case 0xcd: return "Bicycle Crank";
	case 0xce: return "Handle Bars";
	case 0xcf: return "Front Brake";
	case 0xd0: return "Rear Brake";
	default: return "Reserved";
	}
}



const char *vr_controls_page_string(uint32_t id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Belt";
	case 0x02: return "Body Suit";
	case 0x03: return "Flexor";
	case 0x04: return "Glove";
	case 0x05: return "Head Tracker";
	case 0x06: return "Head Mounted Display";
	case 0x07: return "Hand Tracker";
	case 0x08: return "Oculometer";
	case 0x09: return "Vest";
	case 0x0a: return "Animatronic Device";
	case 0x20: return "Stereo Enable";
	case 0x21: return "Display Enable";
	default: return "Reserved";
	}
}



const char *sport_controls_page_string(uint32_t id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Baseball Bat";
	case 0x02: return "Golf Club";
	case 0x03: return "Rowing Machine";
	case 0x04: return "Treadmill";
	case 0x30: return "Oar";
	case 0x31: return "Slope";
	case 0x32: return "Rate";
	case 0x33: return "Stick Speed";
	case 0x34: return "Stick Face Angle";
	case 0x35: return "Stick Heel/Toe";
	case 0x36: return "Stick Follow Through";
	case 0x37: return "Stick Tempo";
	case 0x38: return "Stick Type";
	case 0x39: return "Stick Height";
	case 0x50: return "Putter";
	case 0x51: return "1 Iron";
	case 0x52: return "2 Iron";
	case 0x53: return "3 Iron";
	case 0x54: return "4 Iron";
	case 0x55: return "5 Iron";
	case 0x56: return "6 Iron";
	case 0x57: return "7 Iron";
	case 0x58: return "8 Iron";
	case 0x59: return "9 Iron";
	case 0x5a: return "10 Iron";
	case 0x5b: return "11 Iron";
	case 0x5c: return "Sand Wedge";
	case 0x5d: return "Loft Wedge";
	case 0x5e: return "Power Wedge";
	case 0x5f: return "1 Wood";
	case 0x60: return "3 Wood";
	case 0x61: return "5 Wood";
	case 0x62: return "7 Wood";
	case 0x63: return "9 Wood";
	default: return "Reserved";
	}
}



const char *game_controls_page_string(uint32_t id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "3D Game Controller";
	case 0x02: return "Pinball Device";
	case 0x03: return "Gun Device";
	case 0x20: return "Point of View";
	case 0x21: return "Turn Right/Left";
	case 0x22: return "Pitch Forward/Backward";
	case 0x23: return "Roll Right/Left";
	case 0x24: return "Move Right/Left";
	case 0x25: return "Move Forward/Backward";
	case 0x26: return "Move Up/Down";
	case 0x27: return "Lean Right/Left";
	case 0x28: return "Lean Forward/Backward";
	case 0x29: return "Height of POV";
	case 0x2a: return "Flipper";
	case 0x2b: return "Secondary Flipper";
	case 0x2c: return "Bump";
	case 0x2d: return "New Game";
	case 0x2e: return "Shoot Ball";
	case 0x2f: return "Player";
	case 0x30: return "Gun Bolt";
	case 0x31: return "Gun Clip";
	case 0x32: return "Gun Selectory";
	case 0x33: return "Gun Single Shot";
	case 0x34: return "Gun Burst";
	case 0x35: return "Gun Automatic";
	case 0x36: return "Gun Safety";
	case 0x37: return "Gamepad Fire/Jump";
	case 0x39: return "Gamepad Trigger";
	default: return "Reserved";
	}
}



const char *generic_device_controls_page_string(uint32_t id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x20: return "Battery Strength";
	case 0x21: return "Wireless Channel";
	case 0x22: return "Wireless ID";
	default: return "Reserved";
	}
}



const char *keyboard_keypad_page_string(uint32_t id)
{
	switch (id) {
	case 0x00: return "Undefined";
	case 0x01: return "Keyboard ErrorRollOver";
	case 0x02: return "Keyboard POSTFail";
	case 0x03: return "Keyboard ErrorUndefined";
	case 0x04: return "Keyboard a and A";
	case 0x05: return "Keyboard b and B";
	case 0x06: return "Keyboard c and C";
	case 0x07: return "Keyboard d and D";
	case 0x08: return "Keyboard e and E";
	case 0x09: return "Keyboard f and F";
	case 0x0a: return "Keyboard g and G";
	case 0x0b: return "Keyboard h and H";
	case 0x0c: return "Keyboard i and I";
	case 0x0d: return "Keyboard j and J";
	case 0x0e: return "Keyboard k and K";
	case 0x0f: return "Keyboard l and L";
	case 0x10: return "Keyboard m and M";
	case 0x11: return "Keyboard n and N";
	case 0x12: return "Keyboard o and O";
	case 0x13: return "Keyboard p and P";
	case 0x14: return "Keyboard q and Q";
	case 0x15: return "Keyboard r and R";
	case 0x16: return "Keyboard s and S";
	case 0x17: return "Keyboard t and T";
	case 0x18: return "Keyboard u and U";
	case 0x19: return "Keyboard v and V";
	case 0x1a: return "Keyboard w and W";
	case 0x1b: return "Keyboard x and X";
	case 0x1c: return "Keyboard y and Y";
	case 0x1d: return "Keyboard z and Z";
	case 0x1e: return "Keyboard 1 and !";
	case 0x1f: return "Keyboard 2 and @";
	case 0x20: return "Keyboard 3 and #";
	case 0x21: return "Keyboard 4 and $";
	case 0x22: return "Keyboard 5 and %";
	case 0x23: return "Keyboard 6 and ^";
	case 0x24: return "Keyboard 7 and &";
	case 0x25: return "Keyboard 8 and *";
	case 0x26: return "Keyboard 9 and (";
	case 0x27: return "Keyboard 0 and )";
	case 0x28: return "Keyboard Return";
	case 0x29: return "Keyboard Escape";
	case 0x2a: return "Keyboard Delete (Backspace)";
	case 0x2b: return "Keyboard Tab";
	case 0x2c: return "Keyboard Spacebar";
	case 0x2d: return "Keyboard - and _";
	case 0x2e: return "Keyboard = and +";
	case 0x2f: return "Keyboard [ and {";
	case 0x30: return "Keyboard ] and }";
	case 0x31: return "Keyboard \\ and |";
	case 0x32: return "Keyboard Non-US # and ~";
	case 0x33: return "Keyboard ; and :";
	case 0x34: return "Keyboard ' and \"";
	case 0x35: return "Keyboard Grave Accent and Tilde";
	case 0x36: return "Keyboard , and <";
	case 0x37: return "Keyboard . and >";
	case 0x38: return "Keyboard / and ?";
	case 0x39: return "Keyboard Caps Lock";
	case 0x3a: return "Keyboard F1";
	case 0x3b: return "Keyboard F2";
	case 0x3c: return "Keyboard F3";
	case 0x3d: return "Keyboard F4";
	case 0x3e: return "Keyboard F5";
	case 0x3f: return "Keyboard F6";
	case 0x40: return "Keyboard F7";
	case 0x41: return "Keyboard F8";
	case 0x42: return "Keyboard F9";
	case 0x43: return "Keyboard F10";
	case 0x44: return "Keyboard F11";
	case 0x45: return "Keyboard F12";
	default: return "Reserved";
	}
}



const char *usage_string(uint32_t usage, uint32_t id)
{
	switch (usage) {
	case 0x1: return generic_desktop_page_string(id);
	case 0x2: return simulation_controls_page_string(id);
	case 0x3: return vr_controls_page_string(id);
	case 0x4: return sport_controls_page_string(id);
	case 0x5: return game_controls_page_string(id);
	case 0x6: return generic_device_controls_page_string(id);
	case 0x7: return keyboard_keypad_page_string(id);
	default: return "?????";
	}
}



string input_output_feature_string(main_type type, uint32_t value) {
	string s;
	s += value & 0x01 ? "*const " : "data ";
	s += value & 0x02 ? "*var " : "array ";
	s += value & 0x04 ? "*rel " : "abs ";
	s += value & 0x08 ? "*non-lin " : "lin ";
	s += value & 0x10 ? "*no-pref-state " : "pref-state ";
	s += value & 0x20 ? "*no-null-state " : "null-pos ";
	if (type > 0)
		s += value & 0x40 ? "*vol " : "non-vol ";
	s += value & 0x80 ? "*buff-bytes" : "bit-field";
	return s;
}



string unit_string(uint32_t u)
{
	vector<string> v;
	int i;
	i = u & 0x0f; // nibble 0
	if (i) {
		string s = "System(";
		switch (i) {
		case 1: s += "SI-Linear"; break;
		case 2: s += "SI-Rotation"; break;
		case 3: s += "English-Linear"; break;
		case 4: s += "English-Rotation"; break;
		default: s += "???"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 1
	if (i) {
		string s = "Length(";
		switch (i) {
		case 1: s += "Centimeter"; break;
		case 2: s += "Radians"; break;
		case 3: s += "Inch"; break;
		case 4: s += "Degrees"; break;
		default: s+= "???"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 2
	if (i) {
		string s = "Mass(";
		switch (i) {
		case 1: case 2: s += "Gram"; break;
		case 3: case 4: s += "Slug"; break;
		default: s += "???"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 3
	if (i) {
		string s = "Time(";
		switch (i) {
		case 0: s += "None"; break;
		default: s += "Seconds"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 4
	if (i) {
		string s = "Temperature(";
		switch (i) {
		case 1: case 2: s += "Kelvin"; break;
		case 3: case 4: s += "Fahrenheit"; break;
		default: s += "???"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 5
	if (i) {
		string s = "Current(";
		switch (i) {
		case 0: s += "None"; break;
		default: s += "Ampere"; break;
		}
		v.push_back(s + ')');
	}

	i = (u >>= 4) & 0x0f; // nibble 6
	if (i) {
		string s = "Luminous-intensity=";
		switch (i) {
		case 0: s += "None"; break;
		default: s += "Candela"; break;
		}
		v.push_back(s);
	}

	i = (u >>= 4) & 0x0f; // nibble 7
	if (i)
		log(WARN) << "use of reserved unit nibble 7" << endl;

	return v.empty() ? string("None") : string_join(v);
}



hid_parser::hid_parser() : _item(0), _bitpos(0)
{
	_data_stack.push_back(hid_global_data());
	_global = &_data_stack[0];

	_item = new hid_main_item(ROOT, _bitpos, *_global, _local, _usage);
	_item_stack.push_back(_item);
}



hid_parser::~hid_parser()
{
	delete _item;
}



void hid_parser::parse(const unsigned char *data, int len)
{
	_depth = 0;
	for (const unsigned char *d = data; d < data + len; ) {
		if (*d == 0xfe) { // long item
			int size = *++d;
			/*int tag = * */++d;

			log(ALERT) << ORIGIN"skipping unsupported long item" << endl;
			d += size;

		} else {          // short item
			int size = *d & 0x3;
			if (size == 3)
				size++;

			log(INFO) << dec << setw(3) << d - data << ": " << BBLACK << hexstr(d, 1 + size, 19) << NORM;

			int type = (*d >> 2) & 0x3;
			int tag = (*d++ >> 4) & 0xf;
			uint32_t value = 0;
			if (size > 0)
				value = *d++;
			if (size > 1)
				value |= *d++ << 8;
			if (size > 2)
				value |= *d++ << 16, value |= *d++ << 24;

			if (type == 0) {        // Main
				log(INFO) << MAGENTA;
				do_main(tag, value);
				log(INFO) << NORM << endl;

			} else if (type == 1) { // Global
				log(INFO) << _indent << YELLOW;
				do_global(tag, value);
				log(INFO) << NORM << endl;

			} else if (type == 2) { // Local
				log(INFO) << _indent << CYAN;
				do_local(tag, value);
				log(INFO) << NORM << endl;

			} else {                // Reserved
				log(INFO) << _indent << "Reserved" << endl; // FIXME
				log(ALERT) << ORIGIN"short item: skipping item of reserved type" << endl;
			}
		}
	}
}



void hid_parser::do_main(int tag, uint32_t value)
{
	hid_main_item *current = _item_stack[_item_stack.size() - 1];
	switch (tag) {
	case 0x8:   // Input
		log(INFO) << _indent << "Input " << input_output_feature_string(INPUT, value);
		current->children.push_back(new hid_main_item(INPUT, _bitpos, *_global, _local, _usage));
		break;
	case 0x9:   // Output
		log(INFO) << _indent << "Output " << input_output_feature_string(OUTPUT, value);
		current->children.push_back(new hid_main_item(OUTPUT, _bitpos, *_global, _local, _usage));
		break;
	case 0xb:   // Feature
		log(INFO) << _indent << "Feature " << input_output_feature_string(FEATURE, value);
		current->children.push_back(new hid_main_item(FEATURE, _bitpos, *_global, _local, _usage));
		break;
	case 0xa: { // Collection
			log(INFO) << _indent << "Collection '" << collection_string(value) << '\'';
			_indent.assign(++_depth, '\t');
			hid_main_item *collection = new hid_main_item(COLLECTION, _bitpos, *_global, _local, _usage);
			current->children.push_back(collection);
			_item_stack.push_back(collection);
		}
		break;
	case 0xc: // End Collection
		if (_depth) {
			_indent.assign(--_depth, '\t');
			log(INFO) << _indent << "End Collection ";
			_item_stack.pop_back();
		} else {
			log(ALERT) << "ignoring excess 'End Collection'" << endl;
		}
		break;
	}
	_local.reset();
	_usage.clear();
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
		_data_stack.push_back(hid_global_data(_global));
		_global = &_data_stack[_data_stack.size() - 1];
		return;
	case 0xb:
		log(INFO) << "Pop";
		if (_data_stack.size() > 1) {
			_data_stack.pop_back();
			_global = &_data_stack[_data_stack.size() - 1];
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
	case 0x0: // usage
		if (_global->usage_table >= 0xff00 && _global->usage_table <= 0xffff) // vendor defined
			log(INFO) << "Usage " << value;
		else
			log(INFO) << "Usage '" << usage_string(_global->usage_table, value) << '\'';
		_usage.push_back(value);
		return;
	case 0x1:
		log(INFO) << "Usage Minimum";
		_local.usage_minimum = value;
		break;
	case 0x2:
		log(INFO) << "Usage Maximum";
		_local.usage_maximum = value;
		break;
	case 0x3:
		log(INFO) << "Designator Index";
		_local.designator_index = value;
		break;
	case 0x4:
		log(INFO) << "Designator Minimum";
		_local.designator_minimum = value;
		break;
	case 0x5:
		log(INFO) << "Designator Maximum";
		_local.designator_maximum = value;
		break;
	case 0x6:
		log(INFO) << "???";
		break;
	case 0x7:
		log(INFO) << "String Index";
		_local.string_index = value;
		break;
	case 0x8:
		log(INFO) << "String Minimum";
		_local.string_minimum = value;
		break;
	case 0x9:
		log(INFO) << "String Maximum";
		_local.string_maximum = value;
		break;
	case 0xa:
		log(INFO) << "Delimiter";
		_local.delimiter = value;
		break;
	default:
		log(INFO) << "Reserved";
		break;
	}
	log(INFO) << " = " << value;
}
