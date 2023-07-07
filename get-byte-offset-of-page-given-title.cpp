#include <vector>
#include "get-byte-offset-of-page-given-title.hpp"

int main(const int argc,  const char* const* const argv){
	constexpr std::string_view usage_str =
		"USAGE: [TITLE] [PAGEID]?\n"
	;
	
	if ((argc != 2) and (argc != 3)){
		write(2, usage_str.data(), usage_str.size());
		return 1;
	}
	if (argc == 3){
		write(2, "Not yet implemented PAGEID\n", 27);
		return 1;
	}
	
	pages_articles_multistream_index_txt_offsetted_gz__init();
	char title_str_buf[1+255+1];
	constexpr std::size_t buf_sz = 1024*1024;
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz + get_byte_offset_of_page_given_title::max_line_sz*2));
	const std::string_view result = find_line_containing_title(argv[1], contents,buf_sz, title_str_buf);
	pages_articles_multistream_index_txt_offsetted_gz__deinit();
	write(1, result.data(), result.size());
	
	return 0;
}
