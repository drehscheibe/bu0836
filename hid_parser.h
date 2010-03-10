#include <stdint.h>
#include <string>
#include <vector>



struct hid_report_data {
	hid_report_data() : usage_table(0), logical_minimum(0), logical_maximum(0),
		physical_minimum(0), physical_maximum(0), unit_exponent(0),
		unit(0), report_size(0), report_id(0), report_count(0)
	{}

	hid_report_data(hid_report_data *d) : usage_table(d->usage_table),
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



class hid_parser {
public:
	hid_parser();
	void parse(const unsigned char *data, int len);

private:
	void do_main(int tag, uint32_t value);
	void do_global(int tag, uint32_t value);
	void do_local(int tag, uint32_t value);
	std::vector<hid_report_data> _stack;
	hid_report_data *_global;
	std::string _indent;
};
