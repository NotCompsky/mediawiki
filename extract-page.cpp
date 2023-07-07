#include "extract-page.hpp"

int main(const int argc,  const char* const* const argv){
	constexpr std::string_view usage_str =
		"USAGE: [OPTPUT_TYPE] [OFFSET]:[PAGEID]\n"
		"\n"
		"OPTPUT_TYPE\n"
		"	x   XML\n"
		"	h   HTML\n"
	;
	off_t init_offset_bytes = 0;
	bool extract_as_html = true;
	
	if (unlikely(argc != 3)){
		goto print_usage_and_exit;
	}
	if (argv[1][0] == 'x'){
		extract_as_html = false;
	} else if (argv[1][0] != 'h'){
		goto print_usage_and_exit;
	}
	
	const char* searching_for_pageid__at;
	{
		const char* arg = argv[2];
		while(*arg != ':'){
			if (likely((*arg >= '0') and (*arg <= '9'))){
				init_offset_bytes *= 10;
				init_offset_bytes += *arg - '0';
			} else {
				goto print_usage_and_exit;
			}
			++arg;
		}
		++arg;
		searching_for_pageid__at = arg;
		while(*arg != ':'){
			++arg;
		}
	}
	if (false){
		print_usage_and_exit:
		write(2, usage_str.data(), usage_str.size());
		return 1;
	}
	
	char* const output_buf = reinterpret_cast<char*>(malloc(1024*1024*10));
	const std::string_view errstr(process_file(output_buf, searching_for_pageid__at, init_offset_bytes, nullptr, extract_as_html));
	if (unlikely(errstr.data()[0] == '\0')){
		write(2, errstr.data()+1, errstr.size()-1);
		return 1;
	}
	write(1, errstr.data(), errstr.size());
	
	return 0;
}
