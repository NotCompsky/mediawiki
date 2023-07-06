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
	
	const std::string_view result = find_line_containing_title(argv[1]);
	write(1, result.data(), result.size());
	
	return 0;
}
