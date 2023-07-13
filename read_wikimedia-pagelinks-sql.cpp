// #include <compsky/os/read.hpp>
#include <compsky/macros/likely.hpp>
#include <cstdlib>
#include <cstdio> // malloc
#include <cstdint>
#include <unistd.h>
#include <fcntl.h> // for open, O_RDONLY, O_WRONLY
#include <zlib.h>
#include <vector>
#include <cstring>
#include <string_view>
#include <compsky/asciify/asciify.hpp>
#include <compsky/os/metadata.hpp>
#include "read_wikimedia-page-quickmetadata.hpp"
#include "fast-pagelinks.hpp"

constexpr size_t initial_offset =
	// 1675 if wikidatawiki-20230601-pagelinks.sql
	1668 // if enwiki-20230620-pagelinks.sql
;

constexpr std::size_t pageid_objs_per_group = 3;
constexpr std::size_t compressed_form__buf__sz = sizeof(PageIDOrName1)*1024*400*pageid_objs_per_group;
constexpr unsigned pageids_per_obj = sizeof(PageIDOrName1::as_pageid)/sizeof(uint16_t);
constexpr unsigned pageids_per_pagetitle = pageids_per_obj*(pageid_objs_per_group-1);

constexpr char chrx = 0; // NOTE: Must be 0 because memset entire compressed_form__buf as 0

struct Entry {
	uint32_t pl_from_pageid;
	int8_t pl_namespace;
	int8_t pl_from_namespace;
	uint8_t pl_title_sz;
	char pl_title[255+1];
	void reset(){
		this->pl_from_pageid = 0;
		this->pl_namespace = 0;
		this->pl_from_namespace = 0;
		this->pl_title_sz = 0;
		memset(this->pl_title, chrx, 256); // for add_pageidorname
	}
};

struct Str256 {
	char str[256];
};


uint32_t a2n(const char* s){
	uint32_t n = 0;
	while(*s != 0){
		n *= 10;
		if ((*s > '9') or (*s < '0'))
			return 0;
		n += *s - '0';
		++s;
	}
	return n;
}


bool in_list(const uint32_t value,  const std::vector<uint32_t>& ls){
	for (const uint32_t x : ls){
		if (unlikely(x == value))
			return true;
	}
	return false;
}

bool in_list(char* const entries_from_file__which_have_no_pageids__strbuf,  const char* const title,  const unsigned title_sz,  std::vector<wikipedia::page::StringView>& ls){
	for (const wikipedia::page::StringView& x : ls){
		if (x.sz == title_sz){
			bool matches = true;
			for (unsigned i = 0;  i < title_sz;  ++i){
				matches &= (title[i] == entries_from_file__which_have_no_pageids__strbuf[x.buf_offset+i]);
			}
			if (unlikely(matches))
				return true;
		}
	}
	return false;
}
bool in_list(char* const entries_from_file__which_have_no_pageids__strbuf,  const Entry& entry,  std::vector<wikipedia::page::StringView>& ls){
	return in_list(entries_from_file__which_have_no_pageids__strbuf,  entry.pl_title,  entry.pl_title_sz, ls);
}


void errandexit(const char* const errstr, const char* const buf,  const unsigned i){
	fprintf(stderr, "ERROR: %s\n", errstr);
	fprintf(stderr, "       Previous 300 chars: %.100s\n", buf+((i>300)?i:300)-300);
	fprintf(stderr, "       This char: %c\n", buf[i]);
	fprintf(stderr, "       Next     10  chars: %.10s\n",  buf+i);
	fflush(stdout);
	fflush(stderr);
	abort();
}


void add_pageidorname(PageIDOrName1* save_into_compressed_form__end,  PageIDOrName1** save_into_compressed_form__itr_ptr,  const Entry& entry,  const int fd){
	PageIDOrName1* save_into_compressed_form__itr = *save_into_compressed_form__itr_ptr;
	{
		const unsigned n_elements_thusfar = compsky::utils::ptrdiff(save_into_compressed_form__itr,save_into_compressed_form__end-compressed_form__buf__sz) >> 5;
		//if (n_elements_thusfar > 1227000)
		//	printf("add_pageidorname %u\n", n_elements_thusfar);
		/*if (save_into_compressed_form__itr == save_into_compressed_form__end)
			write(1, "AAAAAAAAAAAAAAAA\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end-1)
			write(1, "AAAAAAAAAAAAAA-1\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end-2)
			write(1, "AAAAAAAAAAAAAA-2\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end-3)
			write(1, "AAAAAAAAAAAAAA-3\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end-4)
			write(1, "AAAAAAAAAAAAAA-4\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end+1)
			write(1, "AAAAAAAAAAAAAA+1\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end+2)
			write(1, "AAAAAAAAAAAAAA+2\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end+3)
			write(1, "AAAAAAAAAAAAAA+3\n", 17);
		if (save_into_compressed_form__itr == save_into_compressed_form__end+4)
			write(1, "AAAAAAAAAAAAAA+4\n", 17);*/
	}
	static unsigned add_pageidorname__current_indx = 0;
	static unsigned tmp_indx = 0;
	
	static_assert(pageids_per_obj == 16);
	constexpr unsigned PPO = pageids_per_obj;
	
	constexpr std::size_t offset_of_each_pagetitle_group = pageid_objs_per_group * sizeof(PageIDOrName1);
	
	if (save_into_compressed_form__itr[0].is_name_same__unaligned(entry.pl_title)){
		save_into_compressed_form__itr[1+(add_pageidorname__current_indx/PPO)].as_pageid[add_pageidorname__current_indx%PPO] = entry.pl_from_pageid;
		++add_pageidorname__current_indx;
		
		if (add_pageidorname__current_indx == pageids_per_pagetitle){
			save_into_compressed_form__itr += pageid_objs_per_group;
			add_pageidorname__current_indx = 0;
		}
	} else {
		if (not save_into_compressed_form__itr[0].is_name_empty()){
			save_into_compressed_form__itr += pageid_objs_per_group;
		}
		
		save_into_compressed_form__itr[0].set_name(entry.pl_title);
		
		save_into_compressed_form__itr[1].as_pageid[0] = entry.pl_from_pageid;
		add_pageidorname__current_indx = 1;
	}
	if (unlikely(save_into_compressed_form__itr == save_into_compressed_form__end)){
		write(2, "Writing to file\n", 16);
		void* const compressed_form__buf = reinterpret_cast<void*>(save_into_compressed_form__end) - compressed_form__buf__sz;
		if (unlikely(write(fd, reinterpret_cast<char*>(compressed_form__buf), compressed_form__buf__sz) != compressed_form__buf__sz)){
			fprintf(stderr, "Failed to write proper number of bytes\n");
			abort();
		}
		save_into_compressed_form__itr = reinterpret_cast<PageIDOrName1*>(compressed_form__buf);
		//for (unsigned i = 0;  i < pageid_objs_per_group*100;  i+=pageid_objs_per_group)
		//	printf("%u %s\n", i, save_into_compressed_form__itr[i].as_name);
		memset(compressed_form__buf, 0, compressed_form__buf__sz);
	}
	*save_into_compressed_form__itr_ptr = save_into_compressed_form__itr;
}


int main(const int argc,  const char* const* const argv){
	constexpr std::string_view usage_str =
		"USAGE: [FILEPATH] [[OPTIONS]] [[PAGE_TITLES]]?\n"
		"\n"
		"FILEPATH\n"
		"	Must be gzipped - either a SQL dump or the 'compressed' version produced by '-w' option\n"
		"OPTIONS\n"
		"	--\n"
		"		End of options\n"
		"	-f [FILENAME]\n"
		"		Use file with FILENAME\n"
		"		All within ~/bin/backupable_from_repos/wikipedia/interesting-wiki-pages/\n"
		"	-i [INTEGERS_CSV]\n"
		"		List links from page ID\n"
		"	-d [DIRECTION]\n"
		"		Default: FROM\n"
		"		Options:\n"
		"			FROM\n"
		"				Get links that are from the listed page IDs\n"
		"			TO\n"
		"				Get links that are to the listed page titles\n"
		"			BOTH\n"
		"			.\n"
		"				Get all links\n"
		"	-c [FILEPATH]\n"
		"		Read from or save to compressed (faster) form, instead of the SQL dump\n"
		"		NOTE: First entry is always zero-chars\n"
		"	-w\n"
		"		Write to: /media/vangelic/DATA/dataset/wikipedia/pagelinks.obj\n"
		"		Saves ALL entries (not just matching entries) into compressed form\n"
		"	-c\n"
		"		Read entries from compressed form (much faster) instead of the SQL dump\n"
		"		Reads from [FILEPATH]\n"
		"PAGE_TITLES\n"
		"	List links TO these titles\n"
		"	Underscores_instead_of_spaces\n"
	;
	const char* errstr1 = nullptr;
	const char* errstr2 = nullptr;
	
	std::vector<uint32_t> links_from_pageids;
	std::vector<wikipedia::page::StringView> links_to_page_titles;
	
	std::vector<wikipedia::page::EntryIndirect> entries_from_file;
	std::vector<wikipedia::page::StringView> entries_from_file__which_have_no_pageids;
	std::vector<wikipedia::page::FileToOffset> entries_from_file__file_indx_offsets;
	char* entries_from_file__which_have_no_pageids__strbuf;
	std::size_t entries_from_file__which_have_no_pageids__strbuf_sz = 1;
	
	for (unsigned i = 2;  i < argc;  ++i){
		entries_from_file__which_have_no_pageids__strbuf_sz += strlen(argv[i]);
	}
	entries_from_file__which_have_no_pageids__strbuf = reinterpret_cast<char*>(malloc(entries_from_file__which_have_no_pageids__strbuf_sz));
	
	unsigned entries_from_file__which_have_no_pageids__strbuf_offset_for_argstrs = 0;
	
	unsigned direction = 0;
	const char* direction_str = "FROM";
	
	const char* compressed_form__write_to_fp = nullptr;
	bool read_from_compressed_form = false;
	PageIDOrName1* compressed_form__buf;
	PageIDOrName1* save_into_compressed_form__itr;
	PageIDOrName1* save_into_compressed_form__end;
	
	{
		unsigned i;
		for (i = 2;  i < argc;  ++i){
			const char* const arg = argv[i];
			if (unlikely((arg[0] == '-') and (arg[1] == '-') and (arg[2] == 0))){
				++i;
				break;
			}
			if ((arg[0] == '-') and (arg[2] == 0)){
				bool is_bad_arg = true;
				switch(arg[1]){
					case 'c':
						read_from_compressed_form = true;
						is_bad_arg = false;
						break;
					case 'w':
						compressed_form__write_to_fp = "/media/vangelic/DATA/dataset/wikipedia/pagelinks.obj";
						is_bad_arg = false;
						break;
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
					case 'd':
						++i;
						if (i != argc){
							direction_str = argv[i];
							is_bad_arg = false;
						}
						break;
					case 'i':
						++i;
						if (i != argc){
							const char* csv = argv[i];
							uint32_t n = 0;
							while(*csv != 0){
								if (*csv == ','){
									links_from_pageids.emplace_back(n);
									n = 0;
								} else if ((*csv >= '0') and (*csv <= '9')){
									n *= 10;
									n += *csv - '0';
								} else {
									[[unlikely]]
									goto print_err_and_exit;
								}
								++csv;
							}
							if (unlikely(n == 0))
								goto print_err_and_exit;
							links_from_pageids.emplace_back(n);
							is_bad_arg = false;
						}
						break;
				}
				if (unlikely(is_bad_arg)){
					goto print_err_and_exit;
				}
			} else {
				break;
			}
		}
		for (;  i < argc;  ++i){
			const char* const arg = argv[i];
			memcpy(entries_from_file__which_have_no_pageids__strbuf+entries_from_file__which_have_no_pageids__strbuf_offset_for_argstrs, arg, strlen(arg));
			links_to_page_titles.emplace_back(entries_from_file__which_have_no_pageids__strbuf_offset_for_argstrs, strlen(arg));
			entries_from_file__which_have_no_pageids__strbuf_offset_for_argstrs += strlen(arg);
		}
	}
	
	if (argc == 1){
		print_err_and_exit:
		if (errstr1 != nullptr)
			write(2, errstr1, strlen(errstr1));
		if (errstr2 != nullptr){
			write(2, errstr2, strlen(errstr2));
			const char newline = '\n';
			write(2, &newline, 1);
		}
		write(2, usage_str.data(), usage_str.size());
		return 1;
	}
	
	static_assert(sizeof(PageIDOrName1) == 32);
	int compressed_form__fd;
	if (read_from_compressed_form or (compressed_form__write_to_fp != nullptr)){
		compressed_form__buf = reinterpret_cast<PageIDOrName1*>(aligned_malloc(compressed_form__buf__sz, 32));
		memset(compressed_form__buf, 0, compressed_form__buf__sz);
		save_into_compressed_form__itr = compressed_form__buf;
		save_into_compressed_form__end = compressed_form__buf + (compressed_form__buf__sz/sizeof(PageIDOrName1));
		if (compressed_form__write_to_fp != nullptr){
			compressed_form__fd = open(
				compressed_form__write_to_fp,
				O_WRONLY|O_CREAT|O_TRUNC,
				S_IRWXU|S_IRGRP
			);
			if (unlikely(compressed_form__fd == -1)){
				errstr1 = "Cannot open file for writing: ";
				errstr2 = compressed_form__write_to_fp;
				goto print_err_and_exit;
			}
		}
	}
	
	for (const wikipedia::page::EntryIndirect& entry : entries_from_file){
		if (entry.pl_pageid != 0)
			links_from_pageids.emplace_back(entry.pl_pageid);
		if (in_list(entries_from_file__which_have_no_pageids__strbuf, entries_from_file__which_have_no_pageids__strbuf+entry.pl_title_offset, entry.pl_title_sz, links_to_page_titles))
			continue;
		if (likely(entries_from_file__which_have_no_pageids__strbuf[entry.pl_title_offset] != '\n'))
			// Stop 'empty' titles (consisting only of "\n") are included in links_to_page_titles
			links_to_page_titles.emplace_back(entry.pl_title_offset, entry.pl_title_sz);
	}
	Str256* links_to_page_titles__str256;
	unsigned links_to_page_titles__str256__sz;
	if (read_from_compressed_form){
		links_to_page_titles__str256__sz = links_to_page_titles.size();
		links_to_page_titles__str256 = reinterpret_cast<Str256*>(aligned_malloc(256*links_to_page_titles.size(), 32));
		memset(links_to_page_titles__str256, chrx, 256*links_to_page_titles.size());
		for (unsigned i = 0;  i < links_to_page_titles.size();  ++i){
			const wikipedia::page::StringView& x = links_to_page_titles[i];
			memcpy(links_to_page_titles__str256[i].str, entries_from_file__which_have_no_pageids__strbuf+x.buf_offset, x.sz);
			//printf(">>>%.*s\n", (int)x.sz, entries_from_file__which_have_no_pageids__strbuf+x.buf_offset);
		}
	}
	
	switch(direction_str[0]){
		case '.': // . -> 3
			++direction;
		case 'B': // BOTH -> 2
			++direction;
		case 'T': // TO -> 1
			++direction;
		case 'F': // FROM -> 0
			break;
		default:
			goto print_err_and_exit;
	}
	if ((direction != 3) and (compressed_form__write_to_fp == nullptr)){
		if ((links_from_pageids.size() == 0) and ((direction&1) == 0)){
			errstr1 = "links_from_pageids is empty\n";
			goto print_err_and_exit;
		}
		if ((links_to_page_titles.size() == 0) and (direction != 0)){
			errstr1 = "links_to_page_titles is empty\n";
			goto print_err_and_exit;
		}
	}
	
	const bool is_using_sql_dump_file = (compressed_form__write_to_fp == nullptr) and (not read_from_compressed_form);
	
	const gzFile fd = gzopen(argv[1], "rb");
	
	if (is_using_sql_dump_file){ // Read from SQL dump
		std::size_t contents_read_into_buf;
		Entry entry;
		unsigned which_field_currently_parsing = 0;
		bool escaped_last_char = false;
		unsigned number_of_single_quotes = 0;
		bool reached_end_of_sql_statement = false;
		
		constexpr std::size_t buf_sz = 4096;
		char contents[buf_sz];
		
		if (unlikely(gzread(fd, contents, initial_offset) != initial_offset)) // Skip first initial_offset bytes
			return 1;
		contents_read_into_buf = gzread(fd, contents, buf_sz);
		while(true){
			for (unsigned i = 0;  i < contents_read_into_buf;  ++i){
				const char c = contents[i];
				// printf("c %c of %u\n", c, which_field_currently_parsing); fflush(stdout);
				if (
					(c == ',') and
					not ((which_field_currently_parsing==2) and ((number_of_single_quotes&1) == 1))
				){
					++which_field_currently_parsing;
					if (which_field_currently_parsing == 4){
						if (unlikely(entry.pl_from_pageid != 0)){
							errandexit("Hasn't been reset yet", contents, i);
						}
						which_field_currently_parsing = 0;
					}
					number_of_single_quotes = 0;
					continue;
				}
				switch(which_field_currently_parsing){
					case 0:
						if (c != '('){
							entry.pl_from_pageid *= 10;
							entry.pl_from_pageid += c - '0';
						}
						break;
					case 1:
						entry.pl_namespace *= 10;
						entry.pl_namespace += c - '0';
						break;
					case 2:
						if (number_of_single_quotes == 0){
							// Beginning of title
							if (unlikely(c != '\'')){
								errandexit("Expected ' but didn't get it", contents, i);
							} else {
								escaped_last_char = false;
								++number_of_single_quotes;
							}
						} else {
							if ((c == '\'') and not (unlikely(escaped_last_char))){
								++number_of_single_quotes;
								if (unlikely((number_of_single_quotes&1) == 1)){
									// The string is: 'foobar''reegee'
									// which is SQL escape for: "foobar'reegee"
									entry.pl_title[entry.pl_title_sz++] = '\'';
								}
							} else {
								if ((unlikely(c == '\\')) and not (unlikely(escaped_last_char))){
									escaped_last_char = true;
								} else {
									entry.pl_title[entry.pl_title_sz++] = c;
									escaped_last_char = false;
								}
							}
						}
						break;
					case 3:
						if (c == ')'){
							if (compressed_form__write_to_fp != nullptr){
								// entry.pl_title[entry.pl_title_sz] = chrx;
								add_pageidorname(save_into_compressed_form__end, &save_into_compressed_form__itr, entry, compressed_form__fd);
							} else {
								if (direction == 3){
									entry.pl_title[entry.pl_title_sz] = '\n';
									write(1, entry.pl_title, entry.pl_title_sz+1);
								} else if (direction != 0){
									if (unlikely(in_list(entries_from_file__which_have_no_pageids__strbuf, entry, links_to_page_titles))){
										char pageid_as_str[11+1];
										char* _itr = pageid_as_str;
										compsky::asciify::asciify(_itr, entry.pl_from_pageid, '\n');
										write(1, pageid_as_str, compsky::utils::ptrdiff(_itr,pageid_as_str));
									}
								}
								if ((direction&1) == 0){
									if (unlikely(in_list(entry.pl_from_pageid, links_from_pageids))){
										entry.pl_title[entry.pl_title_sz] = '\n';
										write(1, entry.pl_title, entry.pl_title_sz+1);
									}
								}
							}
							//if (i%1000 == 0)
							//	printf("%u %.*s\n", i, (int)entry.pl_title_sz, entry.pl_title);
							/*if (unlikely(entry.pl_title_sz == 0)){
								errandexit("Empty title", contents, i);
							}*/
							entry.reset();
						} else if (unlikely(c == ';')){
							reached_end_of_sql_statement = true;
						} else if (unlikely(reached_end_of_sql_statement)){
							if (
								(c == '\n') or
								(c == 'I') or
								(c == 'N') or
								(c == 'S') or
								(c == 'E') or
								(c == 'R') or
								(c == 'T') or
								(c == ' ') or
								(c == 'I') or
								(c == 'N') or
								(c == 'T') or
								(c == 'O') or
								(c == ' ') or
								(c == '`') or
								(c == 'p') or
								(c == 'a') or
								(c == 'g') or
								(c == 'e') or
								(c == 'l') or
								(c == 'i') or
								(c == 'n') or
								(c == 'k') or
								(c == 's') or
								(c == '`') or
								(c == ' ') or
								(c == 'V') or
								(c == 'A') or
								(c == 'L') or
								(c == 'U') or
								(c == 'E') or
								(c == 'S') or
								(c == ' ')
							){
							} else if (c == '('){
								reached_end_of_sql_statement = false; // Reached new SQL statement
								which_field_currently_parsing = 0;
								number_of_single_quotes = 0;
							} else {
								errandexit("Unexpected char when reached_end_of_sql_statement is true", contents, i);
							}
							// End of SQL statement - usually a new one afterwards
						} else if (c == '/'){
							goto reached_end_of_dump;
						} else {
							entry.pl_from_namespace *= 10;
							entry.pl_from_namespace += c - '0';
						}
						break;
				}
			}
			// fprintf(stderr, "contents_read_into_buf %lu\n", contents_read_into_buf);
			contents_read_into_buf = gzread(fd, contents, buf_sz);
			if (unlikely(contents_read_into_buf == 0))
				break;
		}
	} else { // Read from compressed (faster) file
		std::size_t total_bytes_left_to_read = compsky::os::get_file_sz<0>(compressed_form__write_to_fp);
		while(true){
			const std::size_t n_bytes_read = gzread(fd, compressed_form__buf, compressed_form__buf__sz);
			if (unlikely(n_bytes_read != compressed_form__buf__sz)){
				total_bytes_left_to_read -= n_bytes_read;
				if (unlikely(total_bytes_left_to_read != 0)){
					errstr1 = "Failed to read all. Bytes left to read: ";
					errstr2 = reinterpret_cast<char*>(compressed_form__buf);
					char* itr = reinterpret_cast<char*>(compressed_form__buf);
					compsky::asciify::asciify(itr, total_bytes_left_to_read, '\0');
					goto print_err_and_exit;
				}
			}
			total_bytes_left_to_read -= compressed_form__buf__sz;
			const unsigned NN = n_bytes_read / sizeof(PageIDOrName1);
			PageIDOrName1* const pageid_objs = reinterpret_cast<PageIDOrName1*>(compressed_form__buf);
			if ((direction&1) == 0){ // FROM or BOTH
				for (unsigned i = 0;  i < NN;  i += pageid_objs_per_group){
					for (const uint16_t pageid : links_from_pageids){
						for (unsigned j = 1;  j < pageid_objs_per_group;  ++j){
							if (unlikely(pageid_objs[i+j].contains_pageid(pageid))){
								write(1, pageid_objs[i+0].as_name, 32);
								continue;
							}
						}
					}
				}
			}
			if (direction != 0){ // TO or BOTH
				for (unsigned i = 0;  i < NN;  i += pageid_objs_per_group){
					for (unsigned l = 0;  l < links_to_page_titles__str256__sz;  ++l){
						const Str256& pagetitle = links_to_page_titles__str256[l];
						// printf("links_to_page_titles__str256[%u] %s\n", l, pagetitle.str);
						if (unlikely(pageid_objs[i+0].is_name_same(pagetitle.str))){
							char pageids_as_strs[(10+1)*pageids_per_pagetitle];
							char* itr = pageids_as_strs;
							for (unsigned j = 1;  j < pageid_objs_per_group;  ++j){
								for (unsigned k = 0;  k < pageids_per_obj;  ++k){
									if (pageid_objs[i+j].as_pageid[k] == 0)
										break;
									compsky::asciify::asciify(itr, pageid_objs[i+j].as_pageid[k], '\n');
								}
							}
							write(1, pageids_as_strs, compsky::utils::ptrdiff(itr,pageids_as_strs));
							{
								// If pageid_objs[i+pageid_objs_per_group].is_name_same too, then remove from links_to_page_titles__str256 - because it is alphabetically sorted, it will never appear again
								if (likely(i != NN-pageid_objs_per_group)){
									if (not pageid_objs[i+pageid_objs_per_group].is_name_same(pagetitle.str)){
										for (unsigned ll = l;  ll != links_to_page_titles__str256__sz-1;  ++ll)
											links_to_page_titles__str256[ll] = links_to_page_titles__str256[ll+1];
										--links_to_page_titles__str256__sz;
										printf("links_to_page_titles__str256__sz == %u\n", links_to_page_titles__str256__sz);
										if (links_to_page_titles__str256__sz == 0)
											goto reached_end_of_dump;
									}
								}
							}
							break;
						}
					}
				}
			}
		}
	}
	
	reached_end_of_dump:
	
	if (entries_from_file__which_have_no_pageids__strbuf_sz != 1)
		free(entries_from_file__which_have_no_pageids__strbuf);
	
	if (compressed_form__write_to_fp != nullptr)
		close(compressed_form__fd);
	
	return 0;
}
