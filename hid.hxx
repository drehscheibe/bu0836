#ifndef _HID_PARSER_HXX_
#define _HID_PARSER_HXX_

#include <stdint.h>
#include <string>
#include <vector>



const char *usage_string(uint32_t usage, uint32_t id);
std::string hexstr(const unsigned char *p, unsigned int num, size_t width = 0);



struct hid_global_data {
	hid_global_data() : usage_table(0), logical_minimum(0), logical_maximum(0),
		physical_minimum(0), physical_maximum(0), unit_exponent(0),
		unit(0), report_size(0), report_id(0), report_count(0)
	{}

	hid_global_data(hid_global_data *d) : usage_table(d->usage_table), // FIXME
		logical_minimum(d->logical_minimum), logical_maximum(d->logical_maximum),
		physical_minimum(d->physical_minimum), physical_maximum(d->physical_maximum),
		unit(d->unit), report_size(d->report_size), report_id(d->report_id),
		report_count(d->report_count)
	{}

	uint32_t usage_table;
	uint32_t logical_minimum;
	uint32_t logical_maximum;
	uint32_t physical_minimum;
	uint32_t physical_maximum;
	uint32_t unit_exponent;
	uint32_t unit;
	uint32_t report_size;
	uint32_t report_id;
	uint32_t report_count;
};



struct hid_local_data {
	hid_local_data() { reset(); }
	void reset() {
		usage = usage_minimum = usage_maximum = designator_index = designator_minimum
				= designator_maximum = string_index = string_minimum
				= string_maximum = delimiter = 0;
	}

	uint32_t usage;
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
	hid_value(const char *name, int offset, int width) :
		_name(name),
		_byte_offset(offset / 8),
		_bit_offset(offset & 7),
		_width(width),
		_mask((1 << width) - 1)
	{
	}

	uint32_t get_value(const unsigned char *d) const
	{
		d += _byte_offset;
		uint32_t ret = *d++ << (_bit_offset + 24);
		ret |= *d++ << (_bit_offset + 16);
		ret |= *d++ << (_bit_offset + 8);
		ret |= *d++ << _bit_offset;
		return (ret >> (32 - _width)) & _mask;
	}

	const std::string &name() const { return _name; }

private:
	std::string _name;
	int _byte_offset;
	int _bit_offset;
	int _width;
	int _mask;
};



struct hid_main_item {
	hid_main_item(main_type t, int &bitpos, hid_global_data &g, hid_local_data &l,
			std::vector<uint32_t> &usage) :
		type(t), global(g), local(l)
	{
		size_t usize = usage.size();
		for (uint32_t i = 0; i < global.report_count; i++) {
			const char *ustr = i < usize ? usage_string(global.usage_table, usage[i]) : "??";
			values.push_back(hid_value(ustr, bitpos, global.report_size));
			bitpos += global.report_size;
		}
	}

	~hid_main_item()
	{
		std::vector<hid_main_item *>::const_iterator it, end = children.end();
		for (it = children.begin(); it != end; ++it)
			delete *it;
	}

	main_type type;
	hid_global_data global;
	hid_local_data local;
	std::vector<hid_main_item *> children;
	std::vector<hid_value> values;
};



class hid_parser {
public:
	hid_parser();
	~hid_parser();
	void parse(const unsigned char *data, int len);

private:
	void do_main(int tag, uint32_t value);
	void do_global(int tag, uint32_t value);
	void do_local(int tag, uint32_t value);
	std::vector<hid_global_data> _data_stack;
	hid_global_data *_global;
	hid_local_data _local;
	hid_main_item *_item;
	std::vector<hid_main_item *> _item_stack;
	std::vector<uint32_t> _usage;
	std::string _indent;
	int _bitpos;
	int _depth;
};

#endif
