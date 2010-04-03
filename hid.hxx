#ifndef _HID_PARSER_HXX_
#define _HID_PARSER_HXX_

#include <stdint.h>
#include <string>
#include <vector>



struct hid_global_data {
	hid_global_data() :
		usage_table(0),
		logical_minimum(0),
		logical_maximum(0),
		physical_minimum(0),
		physical_maximum(0),
		unit_exponent(0),
		unit(0),
		report_size(0),
		report_id(0),
		report_count(0),
		unit_exponent_factor(1.0)
	{}

	class undef {
	public:
		undef(int v = 0) : _value(v), _defined(false) {}
		bool defined() const { return _defined; }
		bool undefined() const { return !_defined; }
		int32_t operator=(int32_t v) { _defined = true; return _value = v; }
		int32_t operator()(void) const { return _value; }

	private:
		int32_t _value;
		bool _defined;
	};

	uint32_t usage_table;
	int32_t logical_minimum;
	int32_t logical_maximum;
	undef physical_minimum;
	undef physical_maximum;
	undef unit_exponent;
	uint32_t unit;
	uint32_t report_size;
	uint32_t report_id;
	uint32_t report_count;

	double unit_exponent_factor;
};



struct hid_local_data {
	hid_local_data() { reset(); }

	void reset() {
		usage.clear();
		usage_minimum = usage_maximum = designator_index = designator_minimum
				= designator_maximum = string_index = string_minimum
				= string_maximum = delimiter = 0;
	}

	std::vector<uint32_t> usage;
	uint32_t usage_minimum;
	uint32_t usage_maximum;
	uint32_t designator_index;
	uint32_t designator_minimum;
	uint32_t designator_maximum;
	uint32_t string_index;
	uint32_t string_minimum;
	uint32_t string_maximum;
	uint32_t delimiter;
};



enum main_type {
	ROOT, COLLECTION, INPUT, OUTPUT, FEATURE
};



class hid_value {
public:
	hid_value(const std::string &name, unsigned offset, unsigned width) :
		_name(name),
		_byte_offset(offset >> 3),
		_bit_offset(offset & 7),
		_width(width),
		_mask((1 << width) - 1),
		_msb(1 << (width - 1))
	{}

	uint32_t get_unsigned(const unsigned char *d) const
	{
		d += _byte_offset;
		uint32_t ret = uint32_t(*d++);
		if (_width > 8)
			ret |= uint32_t(*d++) << 8;
		if (_width > 16)
			ret |= uint32_t(*d++) << 16;
		if (_width > 24)
			ret |= uint32_t(*d) << 24;
		ret >>= _bit_offset;
		return ret & _mask;
	}

	int32_t get_signed(const unsigned char *d) const
	{
		uint32_t v = get_unsigned(d);
		return _width && v & _msb ? v | ~_mask : v;
	}

	const std::string &name() const { return _name; }

private:
	std::string _name;
	unsigned _byte_offset;
	unsigned _bit_offset;
	unsigned _width;
	uint32_t _mask;
	uint32_t _msb;
};



class hid_main_item {
public:
	hid_main_item(main_type t, uint32_t dt, hid_main_item *parent, hid_global_data &g,
			hid_local_data &l, int &bitpos);
	~hid_main_item();

	main_type type() const { return _type; }
	uint32_t data_type() const { return _data_type; }
	const hid_main_item *parent() const { return _parent; }
	const hid_global_data &global() const { return _global; }
	const hid_local_data &local() const { return _local; }
	std::vector<hid_value> &values() { return _values; }
	std::vector<hid_main_item *> &children() { return _children; }

private:
	main_type _type;
	uint32_t _data_type;
	hid_main_item *_parent;
	hid_global_data _global;
	hid_local_data _local;
	std::vector<hid_main_item *> _children;
	std::vector<hid_value> _values;
};



class hid {
public:
	hid();
	~hid();

	void parse(const unsigned char *data, int len);
	void print_input_report(hid_main_item *, const unsigned char *data);
	const std::vector<hid_main_item *> data() const { return _item_stack; }

private:
	void do_main(int tag, uint32_t value);
	void do_global(int tag, uint32_t value, int32_t svalue);
	void do_local(int tag, uint32_t value);

	std::vector<hid_global_data> _data_stack;
	hid_global_data _global;
	hid_local_data _local;
	hid_main_item *_item;
	std::vector<hid_main_item *> _item_stack;
	std::string _indent;
	int _bitpos;
	int _depth;
};

#endif
