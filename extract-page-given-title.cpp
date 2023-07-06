#include "extract-page.hpp"
#include "get-byte-offset-of-page-given-title.hpp"

int main(const int argc,  const char* const* const argv){
	if (argc != 2){
		write(2, "USAGE: [TITLE]\n", 15);
		return 1;
	}
	const OffsetAndPageid offset_and_pageid(get_byte_offset_and_pageid_given_title(argv[1]));
	if (unlikely(offset_and_pageid.pageid == nullptr)){
		write(2, "Cannot find offset and page_id\n", 31);
		return 1;
	}
	const std::string_view errstr(process_file(offset_and_pageid.pageid, offset_and_pageid.offset, nullptr, true));
	if (unlikely(errstr.data()[0] == '\0')){
		write(2, errstr.data(), errstr.size());
		return 1;
	}
	return 0;
}
