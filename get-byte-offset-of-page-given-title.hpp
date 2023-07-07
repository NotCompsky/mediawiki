#pragma once

#include <compsky/macros/likely.hpp>
#include <cstdlib>
#include <cstdio> // malloc
#include <cstdint>
#include <unistd.h>
#include <fcntl.h> // for open, O_RDONLY, O_WRONLY
#include <zlib.h>
#include <compsky/utils/ptrdiff.hpp>
#include <cstring> // for memcpy
#include <string_view>
#include <compsky/asciify/asciify.hpp>
// #include <zlib.h> replaced by gzguts.hpp
#include "gzguts.hpp"
#include "pages-articles-multistream-index.index.hpp"


constexpr std::size_t max_line_sz = 19+1+10+1+255;

static gz_state pages_articles_multistream_index_txt_offsetted_gz__fd;

void pages_articles_multistream_index_txt_offsetted_gz__init(){
	gz_state& fd = pages_articles_multistream_index_txt_offsetted_gz__fd;
	fd.size = 0;            /* no buffers allocated yet */
    fd.want = 8192*1024;    /* requested buffer size */
    fd.msg = nullptr;       /* no error message yet */
    fd.mode = GZ_READ;
	fd.level = Z_DEFAULT_COMPRESSION;
    fd.strategy = Z_DEFAULT_STRATEGY;
    fd.direct = 1;
	// fd.path = "[hardcoded_path_for_gzip_error_printing]";
	fd.fd = open("/media/vangelic/DATA/dataset/wikipedia/enwiki-20230620-pages-articles-multistream-index.txt.offsetted.gz", O_RDONLY);
}
void pages_articles_multistream_index_txt_offsetted_gz__deinit(){
	gzclose_r(&pages_articles_multistream_index_txt_offsetted_gz__fd);
}

std::string_view find_line_containing_title(const char* const title_requested){
	char* const title_str = reinterpret_cast<char*>(malloc(1+strlen(title_requested)+1));
	title_str[0] = ':';
	memcpy(title_str+1, title_requested, strlen(title_requested));
	title_str[1+strlen(title_requested)] = '\n';
	const char* title_itr = title_str;
	
	constexpr std::size_t buf_sz = 1024*1024; // 4.498 if 1MiB vs 4.503 if 5MiB vs 4.569 if 10MiB
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz + max_line_sz*2));
	
	gz_state& fd = pages_articles_multistream_index_txt_offsetted_gz__fd;
	
	const uint64_t offset_and_next_offset = get_offsets_given_title(title_requested);
	const uint32_t offset = offset_and_next_offset >> 32;
	const uint32_t next_offset = offset_and_next_offset & 0xffffffffu; // TODO: Unused
	
	if (unlikely(fd.fd == -1)){
		write(2, "Cannot open ...index.txt.gz\n", 28);
		return std::string_view(nullptr,0);
	}
	fd.start = lseek(fd.fd, offset, SEEK_SET);
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
		fd.seek = 0;                /* no seek request pending */
		// gz_error(&fd, Z_OK, NULL);  /* clear error */
		fd.x.pos = 0;               /* no uncompressed data yet */
		fd.strm.avail_in = 0;       /* no input data yet */
	}
	
	// fd = gzopen("/media/vangelic/DATA/dataset/wikipedia/enwiki-20230620-pages-articles-multistream-index.txt.gz", "rb");
	
	unsigned which_field_currently_parsing = 0;
	
	int contents_read_into_buf = gzread(&fd, contents, buf_sz);
	unsigned indx;
	if (likely(contents_read_into_buf != -1)){
	
	memset(contents+buf_sz, 0, max_line_sz);
	
	while(true){
		for (indx = 0;  indx < contents_read_into_buf;  ++indx){
			const char c = contents[indx];
			if (unlikely(c == *title_itr)){
				if (unlikely(*title_itr == '\n'))
					goto found_results;
				++title_itr;
			} else {
				title_itr = title_str;
			}
		}
		
		memcpy(contents+buf_sz, contents, max_line_sz);
		contents_read_into_buf = gzread(&fd, contents, buf_sz);
		if (contents_read_into_buf <= 0)
			return std::string_view(nullptr,0);
	}
	}
	
	found_results:
	char* _end_of_line = contents + indx;
	if (unlikely(indx < max_line_sz+1)){
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

off_t get_byte_offset_given_title(const char* const title_requested){
	const std::string_view res = find_line_containing_title(title_requested);
	if (res.size() == 0)
		return -1;
	const char* itr = res.data();
	off_t offset = 0;
	while(*itr != ':'){
		offset *= 10;
		offset += *itr - '0';
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
OffsetAndPageid get_byte_offset_and_pageid_given_title(const char* const title_requested){
	const std::string_view res = find_line_containing_title(title_requested);
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
