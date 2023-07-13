#pragma once

#include <compsky/macros/likely.hpp>
#include <cstdlib>
#include <cstdio> // malloc
#include <cstdint>
#include <unistd.h>
#include <fcntl.h> // for open, O_RDONLY, O_WRONLY
#include <compsky/utils/ptrdiff.hpp>
#include <cstring> // for memcpy
#include <string_view>
#include <compsky/asciify/asciify.hpp>
// #include <zlib.h> replaced by gzguts.hpp
#include "gzguts.hpp"
#include "pages-articles-multistream-index.index.hpp"


#define GZBUFSIZE 8192 // As defined in gzguts.h from zlib - reading the source code, it seems almost all decompression is directly into the user-chosen buffer, not this buffer (fd.want's buffer) - (fd.want's buffer) is used for small reads and for the initial 'LOOK' (determining if a file is GZIP)

namespace get_byte_offset_of_page_given_title {
	constexpr std::size_t max_line_sz = 19+1+10+1+255;
}

static gz_state pages_articles_multistream_index_txt_offsetted_gz__fd;

void pages_articles_multistream_index_txt_offsetted_gz__init(){
	gz_state& fd = pages_articles_multistream_index_txt_offsetted_gz__fd;
	fd.size = 0;            /* no buffers allocated yet */
    fd.want = GZBUFSIZE;    /* requested buffer size */
    fd.mode = GZ_READ;
    fd.direct = 1;
	fd.strm.total_in = 0;
	fd.x.have = 0;
}
void pages_articles_multistream_index_txt_offsetted_gz__deinit(){
	gzclose_r(&pages_articles_multistream_index_txt_offsetted_gz__fd);
}

std::string_view find_line_containing_title(const int _fd,  const char* const title_requested,  char* const gz_contents_buf,  const std::size_t gz_contents_buf_sz,  char* title_str_buf,  const bool is_wikipedia,  const bool must_be_exact_match){
	using get_byte_offset_of_page_given_title::max_line_sz;
	
	if (unlikely(strlen(title_requested) > 255)){
		write(2, "Title too long\n", 15);
		return std::string_view(nullptr,0);
	}
	memcpy(title_str_buf+1, title_requested, strlen(title_requested));
	char* const title_str_buf__end = title_str_buf+1 + strlen(title_requested) - 1;
	const char* title_itr = title_str_buf;
	if (must_be_exact_match){
		title_str_buf[0] = ':';
		++title_str_buf__end;
		*title_str_buf__end = '\n';
	} else {
		title_str_buf = title_str_buf+1;
	}
	
	const std::size_t buf_sz = gz_contents_buf_sz - 1; // 4.498s if 1MiB vs 4.503s if 5MiB vs 4.569s if 10MiB
	char* const contents = gz_contents_buf + 1;
	gz_contents_buf[0] = '\n'; // To ensure first entry works
	
	gz_state& fd = pages_articles_multistream_index_txt_offsetted_gz__fd;
	
	off64_t offset = 0;
	int module_size = INT_MAX;
	if (is_wikipedia){
		const uint64_t offset_and_next_offset = get_offsets_given_title(title_requested);
		offset      = offset_and_next_offset >> 32;
		module_size = offset_and_next_offset & 0xffffffffu;
	}
	
	fd.fd = _fd;
	fd.start = lseek64(fd.fd, offset, SEEK_SET);
	/* NOTE: Source code (gzlib.c from https://www.zlib.net/) confusing has this:
		state->start = LSEEK(state->fd, 0, SEEK_CUR);
		if (state->start == -1) state->start = 0;
	*/
	// equivalent to gzReset(&fd):
	{
		fd.x.have = 0;              /* no output data available */
			fd.eof = 0;             /* not at end of file */
			fd.past = 0;            /* have not read past end yet */
			fd.how = LOOK;          /* look for gzip header */
		// gz_error(&fd, Z_OK, NULL);  /* clear error */
		fd.x.pos = 0;               /* no uncompressed data yet */
		fd.strm.avail_in = 0;       /* no input data yet */
	}
	
	// fd = gzopen("/media/vangelic/DATA/dataset/wikipedia/enwiki-20230620-pages-articles-multistream-index.txt.gz", "rb");
	
	unsigned which_field_currently_parsing = 0;
	
	int contents_read_into_buf = gzread(&fd, contents, buf_sz);
	unsigned indx;
	bool found_in_first_read = true;
	if (likely(contents_read_into_buf > 0)){
	
	memset(contents+buf_sz, 0, max_line_sz);
	
	while(true){
		for (indx = 0;  indx < contents_read_into_buf;  ++indx){
			const char c = contents[indx];
			if (unlikely(c == *title_itr)){
				if (unlikely(title_itr == title_str_buf__end)) // TODO: Ensure file ends with a trailing newline - otherwise the very last page will never be seen
					goto found_results;
				++title_itr;
			} else {
				title_itr = title_str_buf;
			}
		}
		
		found_in_first_read = false;
		module_size -= contents_read_into_buf;
		
		memcpy(contents+buf_sz, contents, max_line_sz);
		contents_read_into_buf = gzread(&fd, contents, buf_sz);
		if ((contents_read_into_buf <= 0) or (module_size <= 0)){
			return std::string_view(nullptr,0);
		}
	}
	} else {
		write(2, "Read 0 bytes from gzread\n", 25);
		return std::string_view(nullptr,0);
	}
	
	found_results:
	char* _end_of_line = contents + indx;
	if ((unlikely(indx < max_line_sz+1)) and (not found_in_first_read)){
		// Possible that we need to consult previously memcpy'd buffer
		memcpy(contents+buf_sz+max_line_sz, contents, indx);
		_end_of_line = contents+buf_sz+max_line_sz + indx;
	}
	char* _itr = _end_of_line - 1;
	while(*_itr != '\n')
		--_itr;
	++_itr;
	return std::string_view(_itr, compsky::utils::ptrdiff(_end_of_line,_itr)+1);
}

off_t get_byte_offset_given_title(const int fd,  const char* const title_requested,  char* const gz_contents_buf,  const std::size_t gz_contents_buf_sz,  char* const title_str_buf,  const bool is_wikipedia){
	const std::string_view res = find_line_containing_title(fd, title_requested, gz_contents_buf,gz_contents_buf_sz, title_str_buf, is_wikipedia, true);
	if (res.size() == 0)
		return -1;
	const char* itr = res.data();
	off_t offset = 0;
	while(*itr != ':'){
		offset *= 10;
		offset += *itr - '0';
		++itr;
	}
	return offset;
}

struct OffsetAndPageid {
	const off_t offset;
	const char* const pageid;
	OffsetAndPageid(const off_t _offset,  const char* const _pageid)
	: offset(_offset)
	, pageid(_pageid)
	{}
};
OffsetAndPageid get_byte_offset_and_pageid_given_title(const int fd,  const char* const title_requested,  char* const gz_contents_buf,  const std::size_t gz_contents_buf_sz,  char* const title_str_buf,  const bool is_wikipedia){
	const std::string_view res = find_line_containing_title(fd, title_requested, gz_contents_buf,gz_contents_buf_sz, title_str_buf, is_wikipedia, true);
	if (res.size() == 0)
		return OffsetAndPageid(-1, nullptr);
	const char* itr = res.data();
	off_t offset = 0;
	while(*itr != ':'){
		offset *= 10;
		offset += *itr - '0';
		++itr;
	}
	++itr;
	/*uint32_t pageid = 0;
	while(*itr != ':'){
		pageid *= 10;
		pageid += *itr - '0';
	}*/
	return OffsetAndPageid(offset, itr);
}
