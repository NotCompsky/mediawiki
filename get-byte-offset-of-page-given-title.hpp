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
#include <zlib.h>

constexpr std::size_t max_line_sz = 19+1+10+1+255;

std::string_view find_line_containing_title(const char* const title_requested,  const uint32_t pageid){
	char* const title_str = reinterpret_cast<char*>(malloc(1+strlen(title_requested)+1));
	title_str[0] = ':';
	memcpy(title_str+1, title_requested, strlen(title_requested));
	title_str[1+strlen(title_requested)] = '\n';
	const char* title_itr = title_str;
	// -------------------------------------------------------
	constexpr std::size_t buf1_sz = 1024*1024 * 10;
	constexpr std::size_t buf_sz = 1024*1024 * 10;
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz));
	
	const int compressed_fd = open("/media/vangelic/DATA/dataset/wikipedia/enwiki-20230620-pages-articles-multistream-index.txt.bz2", O_RDONLY);
	if (unlikely(compressed_fd == 0)){
		write(2, "ERROR: Cannot open index file\n", 30);
		return std::string_view(nullptr,0);
	}
	bz_stream fd;
	memset(&fd, 0, sizeof(fd));
	char* const buf1 = reinterpret_cast<char*>(malloc(buf1_sz));
	if (unlikely(BZ2_bzDecompressInit(&fd, 0, 0 /*int: variable called 'small' for lower-memory decompression*/) != BZ_OK)){
		write(2, "ERROR: BZ2_bzDecompressInit\n", 28);
		return std::string_view(nullptr,0);
	}
	fd.next_in = buf1;
	fd.avail_in = buf1_sz;
	fd.next_out = contents;
	fd.avail_out = buf_sz;
	
	if (pageid != 0){
		// TODO
		if (unlikely(lseek(compressed_fd, init_offset_bytes, SEEK_SET) != init_offset_bytes)){
			write(2, "ERROR: lseek\n", 13);
			return std::string_view(nullptr,0);
		}
	}
	fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
	int bz2_decompress_rc = BZ2_bzDecompress(&fd);
	if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
		write(2, "ERROR: BZ2_bzDecompress\n", 24);
		return std::string_view(nullptr,0);
	}
	
	char* _start_of_this_page = nullptr;
	std::size_t _start_of_this_page_sz;
	
	unsigned which_field_currently_parsing = 0;
	
	std::size_t contents_read_into_buf = gzread(fd, contents, buf_sz);
	memset(contents+buf_sz, 0, max_line_sz);
	
	unsigned indx;
	while(true){
		for (unsigned i = 0;  i < buf_sz - fd.avail_out;  ++i){
			const char c = contents[i];
			if (unlikely(c == *title_itr)){
				if (unlikely(*title_itr == '\n'))
					goto found_results;
				++title_itr;
			} else {
				title_itr = title_str;
			}
		}
		
		if (bz2_decompress_rc == BZ_STREAM_END)
			break;
		fd.avail_out = buf_sz;
		fd.next_out = contents;
		if (fd.avail_in == 0){
			fd.next_in = buf1;
			fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
		}
		bz2_decompress_rc = BZ2_bzDecompress(&fd);
		if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
			write(2, "ERROR: BZ2_bzDecompress\n", 24);
			goto extraction_failed;
		}
	}
	
	
	
	finish_extraction:
	
	BZ2_bzDecompressEnd(&fd);
	close(compressed_fd);
	
	return std::string_view(nullptr,0);
	
	
	
	found_results:
	
	BZ2_bzDecompressEnd(&fd);
	close(compressed_fd);
	
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
