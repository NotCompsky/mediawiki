#include "extract-page.hpp"
#include "get-byte-offset-of-page-given-title.hpp"

int main(const int argc,  const char* const* const argv){
	if (argc != 2){
		write(2, "USAGE: [TITLE]\n", 15);
		return 1;
	}
	pages_articles_multistream_index_txt_offsetted_gz__init();
	char title_str_buf[1+255+1];
	constexpr std::size_t buf_sz = 1024*1024;
	char* const gz_contents_buf = reinterpret_cast<char*>(malloc(buf_sz + get_byte_offset_of_page_given_title::max_line_sz*2));
	const OffsetAndPageid offset_and_pageid(get_byte_offset_and_pageid_given_title(argv[1], gz_contents_buf,buf_sz, title_str_buf));
	pages_articles_multistream_index_txt_offsetted_gz__deinit();
	if (unlikely(offset_and_pageid.pageid == nullptr)){
		write(2, "Cannot find offset and page_id\n", 31);
		return 1;
	}
	char* const output_buf = reinterpret_cast<char*>(malloc(1024*1024*10));
	const std::string_view errstr(process_file(output_buf, offset_and_pageid.pageid, offset_and_pageid.offset, nullptr, true));
	if (unlikely(errstr.data()[0] == '\0')){
		write(2, errstr.data(), errstr.size());
		return 1;
	}
	return 0;
}
