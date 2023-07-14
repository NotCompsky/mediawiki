#include <compsky/os/read.hpp>
#include <compsky/macros/likely.hpp>
#include <cstdlib>
#include <cstdio> // malloc
#include <cstdint>
#include <unistd.h>
#include <fcntl.h> // for open, O_RDONLY, O_WRONLY
#include <zlib.h>
#include <compsky/utils/ptrdiff.hpp>
#include <vector>
#include <cstring> // for memcpy
#include <string_view>
#include <compsky/asciify/asciify.hpp>
#include "read_wikimedia-page-quickmetadata.hpp"

#ifdef USE_BZIP2
# define BZ_NO_STDIO // restricts bzip2 library
# include <bzlib.h>
#else
# include <zlib.h>
#endif

struct BEntry {
	uint32_t byte_offset;
	uint32_t pl_pageid;
	uint8_t pl_title_sz;
	char pl_title[255];
	void reset(){
		byte_offset = 0;
		pl_pageid = 0;
		pl_title_sz = 0;
	}
};

int main(const int argc,  const char* const* const argv){
	using namespace wikipedia::page;
	
	constexpr std::string_view usage_str =
		"USAGE: [FILEPATH] [[OPTIONS]]\n"
		"\n"
		"FILEPATH\n"
		"	Path to pages-articles-multistream-index.txt.(gz|bz2)\n"
		"\n"
		"OPTIONS\n"
		"	-f [FILENAME]\n"
		"		Use file with FILENAME\n"
		"		All within ~/bin/backupable_from_repos/wikipedia/interesting-wiki-pages/\n"
		"	-l\n"
		"		List all page_ids\n"
	;
	const char* errstr1 = nullptr;
	const char* errstr2 = nullptr;
	
	constexpr std::size_t buf1_sz = 1024*1024 * 10;
	constexpr std::size_t buf_sz = 1024*1024 * 10;
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz));
	
	const char* const filepath = argv[1];
	
#ifdef USE_BZIP2
	const int compressed_fd = open(filepath, O_RDONLY);
#endif
	
	std::vector<EntryIndirect> entries_from_file;
	std::vector<StringView> entries_from_file__which_have_no_pageids;
	std::vector<FileToOffset> entries_from_file__file_indx_offsets;
	char* entries_from_file__which_have_no_pageids__strbuf;
	std::size_t entries_from_file__which_have_no_pageids__strbuf_sz = 1;
	unsigned entries_from_file__which_have_no_pageids__strbuf_itroffset = 0;
	
	bool list_all_pageids_and_exit = false;
	
#ifdef USE_BZIP2
	bz_stream fd;
	memset(&fd, 0, sizeof(fd));
	char* const buf1 = reinterpret_cast<char*>(malloc(buf1_sz));
	if (unlikely(BZ2_bzDecompressInit(&fd, 0, 0 /*int: variable called 'small' for lower-memory decompression*/) != BZ_OK)){
		errstr1 = "BZ2_bzDecompressInit error\n";
		goto print_err_and_exit;
	}
	fd.next_in = buf1;
	fd.avail_in = buf1_sz;
	fd.next_out = contents;
	fd.avail_out = buf_sz;
#else
	const gzFile fd = gzopen(filepath, "rb");
#endif
	
	for (unsigned i = 2;  i < argc;  ++i){
		const char* const arg = argv[i];
		bool is_bad_arg = true;
		if ((arg[0] == '-') and (arg[2] == 0)){
			switch(arg[1]){
				case 'f': {
					++i;
					if (i != argc){
						entries_from_file__file_indx_offsets.emplace_back(entries_from_file.size(), argv[i]);
						errstr1 = parse::parse_pageid2title_file(
							errstr2,
							argv[i],
							entries_from_file__which_have_no_pageids__strbuf,
							entries_from_file__which_have_no_pageids__strbuf_sz,
							entries_from_file,
							entries_from_file__which_have_no_pageids
						);
						if (unlikely(errstr1 != nullptr))
							goto print_err_and_exit;
						is_bad_arg = false;
					}
					break;
				}
				case 'l':
					list_all_pageids_and_exit = true;
					is_bad_arg = false;
					break;
			}
		}
		if (unlikely(is_bad_arg)){
			errstr1 = "Bad argument: ";
			errstr2 = arg;
			goto print_err_and_exit;
		}
	}
	if (false){
		print_err_and_exit:
		write(2, errstr1, strlen(errstr1));
		if (errstr2 != nullptr){
			write(2, errstr2, strlen(errstr2));
			const char newline = '\n';
			write(2, &newline, 1);
		}
		write(2, usage_str.data(), usage_str.size());
		return 1;
	}
	if (list_all_pageids_and_exit){
		if (likely(entries_from_file.size() != 0)){
			char* const _buf = reinterpret_cast<char*>(malloc(entries_from_file.size()*(10+1)+1));
			char* _itr = _buf;
			for (const EntryIndirect& entry : entries_from_file){
				if (entry.pl_pageid != 0)
					compsky::asciify::asciify(_itr, entry.pl_pageid, ',');
			}
			_itr[-1] = '\n';
			write(1, _buf, compsky::utils::ptrdiff(_itr,_buf));
			free(_buf);
		}
		return 0;
	}
	
	EntryIndirect* const entries_from_file__original_order = reinterpret_cast<EntryIndirect*>(malloc(sizeof(EntryIndirect)*entries_from_file.size()));
	memcpy(entries_from_file__original_order, entries_from_file.data(), sizeof(EntryIndirect)*entries_from_file.size());
	std::sort(
		entries_from_file.data(),
		entries_from_file.data()+entries_from_file.size(),
		[](const EntryIndirect& a,  const EntryIndirect& b){
			return (a.pl_pageid < b.pl_pageid); // Sort in ascending order
		}
	);
	
	{
		unsigned n_entries_without_titles = 0;
		for (EntryIndirect& x : entries_from_file){
			if (x.pl_title_sz == 0){
				++n_entries_without_titles;
			}
		}
		if (n_entries_without_titles != 0){
			const unsigned add_mem = 255 * n_entries_without_titles;
			char* const new_buf = reinterpret_cast<char*>(malloc(entries_from_file__which_have_no_pageids__strbuf_sz + add_mem));
			memcpy(new_buf, entries_from_file__which_have_no_pageids__strbuf, entries_from_file__which_have_no_pageids__strbuf_sz);
			if (entries_from_file__which_have_no_pageids__strbuf_sz != 1)
				free(entries_from_file__which_have_no_pageids__strbuf);
			entries_from_file__which_have_no_pageids__strbuf = new_buf;
			entries_from_file__which_have_no_pageids__strbuf_sz += add_mem;
		}
	}
	
	BEntry entry;
	unsigned which_field_currently_parsing = 0;
	bool escaped_last_char = false;
	unsigned number_of_single_quotes = 0;
	bool reached_end_of_sql_statement = false;
	
	const char* expecting_next_chars = nullptr;
	
	std::size_t contents_read_into_buf;
#ifdef USE_BZIP2
	fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
	contents_read_into_buf = buf_sz - fd.avail_out;
	int bz2_decompress_rc = BZ2_bzDecompress(&fd);
	if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
		errstr1 = "first BZ2_bzDecompress error\n";
		goto print_err_and_exit;
	}
#else
	contents_read_into_buf = gzread(fd, contents, buf_sz);
#endif
	
	uint64_t n_read_bytes = 0;
	unsigned entries_from_file__offset = 0;
	while(true){
		for (unsigned i = 0;  i < contents_read_into_buf;  ++i){
			const char c = contents[i];
			// printf("c %c of %u\n", c, which_field_currently_parsing); fflush(stdout);
			switch(which_field_currently_parsing){
				case 0:
					if (c == ':'){
						++which_field_currently_parsing;
					} else {
						entry.byte_offset *= 10;
						entry.byte_offset += c - '0';
					}
					break;
				case 1:
					if (c == ':'){
						++which_field_currently_parsing;
					} else {
						entry.pl_pageid *= 10;
						entry.pl_pageid += c - '0';
					}
					break;
				case 2:
					if (c == '\n'){
						if (entries_from_file__which_have_no_pageids.size() != 0){
							const uint64_t _title_offset_and_sz = in_list(entries_from_file__which_have_no_pageids__strbuf, entry.pl_title, entry.pl_title_sz, entries_from_file__which_have_no_pageids);
							if (_title_offset_and_sz != 0){
								const uint32_t _title_offset = _title_offset_and_sz >> 8;
								const uint8_t _title_sz = _title_offset_and_sz & 255;
								wikipedia::page::update_pageid2title_ls(entries_from_file__which_have_no_pageids__strbuf, entry.pl_pageid, entries_from_file, _title_offset, _title_sz);
								// printf("%u %.*s\n", (unsigned)entry.pl_pageid, (int)entry.pl_title_sz, entry.pl_title);
							}
						}
						for (unsigned k = entries_from_file__offset;  k < entries_from_file.size();  ++k){
							EntryIndirect& x = entries_from_file[k];
							if (unlikely(x.pl_pageid < entry.pl_pageid)){
								++entries_from_file__offset;
								fprintf(stderr, "Found %u / %lu\n", entries_from_file__offset, entries_from_file.size());
								if (unlikely(entries_from_file__offset == entries_from_file.size()))
									goto break2;
								continue;
							}
							if (x.pl_pageid > entry.pl_pageid)
								break;
							{
								[[unlikely]]
								if (x.pl_title_sz == 0){
									x.pl_title_sz = entry.pl_title_sz;
									memcpy(entries_from_file__which_have_no_pageids__strbuf + entries_from_file__which_have_no_pageids__strbuf_itroffset, entry.pl_title, entry.pl_title_sz);
									x.pl_title_offset = entries_from_file__which_have_no_pageids__strbuf_itroffset;
									entries_from_file__which_have_no_pageids__strbuf_itroffset += entry.pl_title_sz;
									break;
								}
							}
						}
						entry.reset();
						which_field_currently_parsing = 0;
					} else {
						entry.pl_title[entry.pl_title_sz++] = c;
					}
					break;
			}
		}
		
		break2:
#ifdef USE_BZIP2
		if (bz2_decompress_rc == BZ_STREAM_END)
			break;
		fd.avail_out = buf_sz;
		fd.next_out = contents;
		if (fd.avail_in == 0){
			n_read_bytes += buf1_sz;
			printf("Read %lu orig bytes\r", (uint64_t)n_read_bytes);
			fd.next_in = buf1;
			fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
		}
		bz2_decompress_rc = BZ2_bzDecompress(&fd);
		if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
			errstr1 = "BZ2_bzDecompress error\n";
			goto print_err_and_exit;
		}
#else
		contents_read_into_buf = gzread(fd, contents, buf_sz);
		if (contents_read_into_buf == 0)
			break;
#endif
	}
	
#ifdef USE_BZIP2
	BZ2_bzDecompressEnd(&fd);
	close(compressed_fd);
#endif
	
	for (unsigned i = 0;  i < entries_from_file.size();  ++i){
		const EntryIndirect& entry = entries_from_file__original_order[i];
		for (const EntryIndirect& entry2 : entries_from_file){
			if (entry2.pl_pageid == entry.pl_pageid){
				if (entry.is_wiki_page){
					if (entry.pl_pageid != 0)
						printf("%u ", entry.pl_pageid);
					else
						printf("\t");
				}
				printf("%.*s\n", (int)entry2.pl_title_sz, entries_from_file__which_have_no_pageids__strbuf+entry2.pl_title_offset);
				break;
			}
		}
	}
	
	if (entries_from_file__which_have_no_pageids__strbuf_sz != 1)
		free(entries_from_file__which_have_no_pageids__strbuf);
	
	return 0;
}
