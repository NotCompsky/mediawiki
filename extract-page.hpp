#pragma once

#include <compsky/os/read.hpp>
#include <compsky/macros/likely.hpp>
#include <cstdlib>
#include <cstdio> // malloc
#include <cstdint>
#include <unistd.h>
#include <fcntl.h> // for open, O_RDONLY, O_WRONLY
#include <compsky/utils/ptrdiff.hpp>
#include <vector>
#include <cstring> // for memcpy
#include <string_view>
#include <compsky/asciify/asciify.hpp>
#include "read_wikimedia-page-quickmetadata.hpp"
#define BZ_NO_STDIO // restricts bzip2 library
#include <bzlib.h>
#include "wikitext_to_html.hpp"

namespace compsky_wiki_extractor {

/*
struct IndxAndUrl {
	const std::string_view url;
	const unsigned indx;
	IndxAndUrl(const std::string_view _url,  const unsigned _indx)
	: url(_url)
	, indx(_indx)
	{}
	IndxAndUrl(char* const _start,  const unsigned _sz,  const unsigned _indx)
	: url(_start, _sz)
	, indx(_indx)
	{}
};*/
struct IndxAndUrlOffset {
	const unsigned start_offset;
	const unsigned end_offset;
	const unsigned indx;
	IndxAndUrlOffset(const unsigned _start_offset,  const unsigned _end_offset,  const unsigned _indx)
	: start_offset(_start_offset)
	, end_offset(_end_offset)
	, indx(_indx)
	{}
};

std::string_view process_file(
	char* const contents, // pre-allocated buffer for decompressed file output
	char* const buf1, // pre-allocated buffer for compressed file input
	const unsigned int contentsbuf_sz,
	const unsigned int buf1_sz,
	const int compressed_fd,
	char* const output_buf, // for desired output
	const char* searching_for_pageid__as_str,
	const off_t init_offset_bytes,
	uint32_t* all_citation_urls,
	const bool extract_as_html
	// returns either an error string or a substring of output_buf
){
	char searching_for_pageid__buf[4+10+5+1];
	char* searching_for_pageid = searching_for_pageid__buf;
	{
		char* _itr = searching_for_pageid__buf;
		while((*searching_for_pageid__as_str >= '0') and (*searching_for_pageid__as_str <= '9')){
			*(_itr++) = *searching_for_pageid__as_str;
			++searching_for_pageid__as_str;
		}
		compsky::asciify::asciify(_itr, '<','/','i','d','>', '\0');
	}
	
	const char* searching_for_pageend__buf = "</text>\n";
	const char* const searching_for_pageid_start__buf = "<id>";
	if (extract_as_html){
		searching_for_pageend__buf = "\n  </page>\n";
	}
	
	bz_stream fd;
	memset(&fd, 0, sizeof(fd));
	if (unlikely(BZ2_bzDecompressInit(&fd, 0, 0 /*int: variable called 'small' for lower-memory decompression*/) != BZ_OK)){
		return std::string_view("\0ERROR: BZ2_bzDecompressInit\n", 29);
	}
	fd.next_in = buf1;
	fd.avail_in = buf1_sz;
	fd.next_out = contents;
	fd.avail_out = contentsbuf_sz;
	
	if (unlikely(lseek(compressed_fd, init_offset_bytes, SEEK_SET) != init_offset_bytes)){
		return std::string_view("\0ERROR: lseek\n", 14);
	}
	fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
	int bz2_decompress_rc = BZ2_bzDecompress(&fd);
	if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
		return std::string_view("\0ERROR: BZ2_bzDecompress\n", 25);
	}
	
	char* _start_of_this_page = nullptr;
	std::size_t _start_of_this_page_sz;
	
	bool found_pageid = false;
	bool found_pageid_start = false;
	bool success = false;
	char* output_itr = output_buf - 1;
	const char* searching_for_pageend = searching_for_pageend__buf;
	const char* searching_for_pageid_start = searching_for_pageid_start__buf;
	while(true){
		for (unsigned i = 0;  i < contentsbuf_sz - fd.avail_out;  ++i){
			const char c = contents[i];
			*(++output_itr) = c;
			if (not found_pageid){
				if (unlikely(found_pageid_start)){
					if (c == *searching_for_pageid){
						++searching_for_pageid;
						if (*searching_for_pageid == 0){
							found_pageid = true;
						}
					} else {
						searching_for_pageid = searching_for_pageid__buf;
						searching_for_pageid_start = searching_for_pageid_start__buf;
						found_pageid_start = false;
						output_itr = output_buf - 1;
					}
				} else if (unlikely(c == *searching_for_pageid_start)){
					++searching_for_pageid_start;
					if (*searching_for_pageid_start == 0){
						found_pageid_start = true;
					}
				} else {
					searching_for_pageid_start = searching_for_pageid_start__buf;
				}
			} else {
				if (unlikely(c == *searching_for_pageend)){
					++searching_for_pageend;
					if (*searching_for_pageend == 0){
						success = true;
						_start_of_this_page = output_buf;
						
						while(true){
							if (
								(_start_of_this_page[0] == '<') and
								(_start_of_this_page[1] == 'p') and
								(_start_of_this_page[2] == 'a') and
								(_start_of_this_page[3] == 'g') and
								(_start_of_this_page[4] == 'e') and
								(_start_of_this_page[5] == '>') and
								(_start_of_this_page[6] == '\n')
							)
								break;
							++_start_of_this_page;
						}
						
						// write(2, output_buf, compsky::utils::ptrdiff(output_itr,_start_of_this_page));
						
						if (extract_as_html){
							while(true){
								if (
									(_start_of_this_page[0] == '<') and
									(_start_of_this_page[1] == 't') and
									(_start_of_this_page[2] == 'e') and
									(_start_of_this_page[3] == 'x') and
									(_start_of_this_page[4] == 't') and
									(_start_of_this_page[5] == ' ')
								)
									break;
								++_start_of_this_page;
							}
							_start_of_this_page += 6;
							while(*_start_of_this_page != '>')
								++_start_of_this_page;
							++_start_of_this_page;
							
							while(true){
								if (unlikely(output_itr == _start_of_this_page)){
									break;
								}
								if (
									(output_itr[0] == '<') and
									(output_itr[1] == '/') and
									(output_itr[2] == 't') and
									(output_itr[3] == 'e') and
									(output_itr[4] == 'x') and
									(output_itr[5] == 't') and
									(output_itr[6] == '>')
								)
									break;
								--output_itr;
							}
						}
						
						if (extract_as_html){
							const std::size_t sz = compsky::utils::ptrdiff(output_itr,_start_of_this_page);
							for (unsigned i = 0;  i < sz;  ++i)
								output_buf[i] = _start_of_this_page[i];
								// because I think maybe this would overwrite itself: memcpy(output_buf, _start_of_this_page, sz);
							_start_of_this_page = output_buf + sz;
							output_itr = wikitext_to_html<std::vector<std::string_view>>(_start_of_this_page, output_buf, all_citation_urls);
						}
						
						_start_of_this_page_sz = compsky::utils::ptrdiff(output_itr,_start_of_this_page);
						goto finish_extraction;
					}
				} else {
					searching_for_pageend = searching_for_pageend__buf;
				}
			}
		}
		
		if (bz2_decompress_rc == BZ_STREAM_END)
			break;
		fd.avail_out = contentsbuf_sz;
		fd.next_out = contents;
		if (fd.avail_in == 0){
			fd.next_in = buf1;
			fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
			if (unlikely(fd.avail_in == 0)){
				return std::string_view("\0ERROR: Reached end of file?", 30);
			}
		}
		bz2_decompress_rc = BZ2_bzDecompress(&fd);
		if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
			return std::string_view("\0ERROR: BZ2_bzDecompress\n", 25);
		}
	}
	
	finish_extraction:
	
	BZ2_bzDecompressEnd(&fd);
	
	return std::string_view(_start_of_this_page, _start_of_this_page_sz);
}

std::string_view process_file(const int compressed_fd,  char* const output_buf,  const char* searching_for_pageid__as_str,  const off_t init_offset_bytes,  uint32_t* all_citation_urls,  const bool extract_as_html){
	constexpr std::size_t buf1_sz = 1024*1024 * 10;
	constexpr std::size_t contentsbuf_sz = 1024*1024 * 10;
	
	char* const contents = reinterpret_cast<char*>(malloc(contentsbuf_sz));
	char* const buf1 = reinterpret_cast<char*>(malloc(buf1_sz));
	const std::string_view res(process_file(
		contents,
		buf1,
		contentsbuf_sz,
		buf1_sz,
		compressed_fd,
		output_buf,
		searching_for_pageid__as_str,
		init_offset_bytes,
		all_citation_urls,
		extract_as_html
	));
	free(buf1);
	free(contents);
	return res;
}

}
