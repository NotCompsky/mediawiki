#include <vector>
#include "get-byte-offset-of-page-given-title.hpp"

int main(const int argc,  const char* const* const argv){
	constexpr std::string_view usage_str =
		"USAGE: [INDICES_FILEPATH] [IS_WIKIPEDIA(0,1)] [TITLE] [PAGEID]?\n"
	;
	
	if ((argc != 4) and (argc != 5)){
		write(2, usage_str.data(), usage_str.size());
		return 1;
	}
	if (argc == 5){
		write(2, "Not yet implemented PAGEID\n", 27);
		return 1;
	}
	
	const char* const filepath = argv[1];
	const bool is_wikipedia    = (argv[2][0] == '1');
	
	const int fd = open(filepath, O_RDONLY);
	if (unlikely(fd == -1)){
		write(2, "Cannot open file\n", 17);
		return 1;
	}
	
	pages_articles_multistream_index_txt_offsetted_gz__init();
	char title_str_buf[1+255+1];
	constexpr std::size_t buf_sz = 1024*1024;
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz + get_byte_offset_of_page_given_title::max_line_sz*2));
	const std::string_view result = find_line_containing_title(fd, argv[2], contents,buf_sz, title_str_buf, is_wikipedia, true);
	pages_articles_multistream_index_txt_offsetted_gz__deinit();
	write(1, result.data(), result.size());
	
	return 0;
}
