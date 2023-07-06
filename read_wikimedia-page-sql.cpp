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
#include "read_wikimedia-page-sql.hpp"

constexpr size_t initial_offset =
	2115 // if enwiki-20230620-page.sql
;


int main(const int argc,  const char* const* const argv){
	using namespace wikipedia::page;
	
	constexpr std::string_view depreciated_msg = "NOTE: Recommend using 'enwiki-20230620-pages-articles-multistream-index.txt.bz2', which contains same information but is much faster\n";
	write(2, depreciated_msg.data(), depreciated_msg.size());
	
	constexpr std::string_view usage_str =
		"USAGE: [[OPTIONS]]\n"
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
	
	const char* const filepath = "/media/vangelic/DATA/dataset/wikipedia/enwiki-20230620-page.sql.gz"; // argv[1];
	const gzFile fd = gzopen(filepath, "rb");
	
	std::vector<EntryIndirect> entries_from_file;
	std::vector<StringView> entries_from_file__which_have_no_pageids;
	std::vector<FileToOffset> entries_from_file__file_indx_offsets;
	char* entries_from_file__which_have_no_pageids__strbuf;
	std::size_t entries_from_file__which_have_no_pageids__strbuf_sz = 1;
	unsigned entries_from_file__which_have_no_pageids__strbuf_itroffset = 0;
	
	bool list_all_pageids_and_exit = false;
	
	for (unsigned i = 1;  i < argc;  ++i){
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
		char* const _buf = reinterpret_cast<char*>(malloc(entries_from_file.size()*(10+1)+1));
		char* _itr = _buf;
		for (const EntryIndirect& entry : entries_from_file){
			if (entry.pl_pageid != 0)
				compsky::asciify::asciify(_itr, entry.pl_pageid, ',');
		}
		_itr[-1] = '\n';
		write(1, _buf, compsky::utils::ptrdiff(_itr,_buf));
		free(_buf);
		return 0;
	}
	
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
	
	constexpr std::size_t buf_sz = 4096;
	char buf[buf_sz];
	
	std::size_t contents_read_into_buf;
	Entry entry;
	unsigned which_field_currently_parsing = 0;
	bool escaped_last_char = false;
	unsigned number_of_single_quotes = 0;
	bool reached_end_of_sql_statement = false;
	
	const char* expecting_next_chars = nullptr;
	
	char contents[buf_sz];
	if (unlikely(gzread(fd, contents, initial_offset) != initial_offset)) // Skip first initial_offset bytes
		return 1;
	contents_read_into_buf = gzread(fd, contents, buf_sz);
	while(true){
		// printf("fd.total_in\r", fd->total_in);
		for (unsigned i = 0;  i < contents_read_into_buf;  ++i){
			const char c = contents[i];
			// printf("c %c of %u\n", c, which_field_currently_parsing); fflush(stdout);
			if (
				(c == ',') and
				not ((which_field_currently_parsing==2) and (number_of_single_quotes&1 == 1))
			){
				++which_field_currently_parsing;
				if (which_field_currently_parsing == 12){
					if (unlikely(entry.pl_pageid != 0)){
						return errandexit("Hasn't been reset yet", contents, i);
					}
					which_field_currently_parsing = 0;
				}
				number_of_single_quotes = 0;
				continue;
			}
			switch(which_field_currently_parsing){
				case 0:
					if (c != '('){
						entry.pl_pageid *= 10;
						entry.pl_pageid += c - '0';
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
							return errandexit("Expected ' but didn't get it", contents, i);
						} else {
							escaped_last_char = false;
							++number_of_single_quotes;
						}
					} else {
						if ((c == '\'') and not (unlikely(escaped_last_char))){
							++number_of_single_quotes;
							if (unlikely(number_of_single_quotes&1 == 1)){
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
				case 11:
					if (c == ')'){
						if (entries_from_file__which_have_no_pageids.size() != 0){
							const uint64_t _title_offset_and_sz = in_list(entries_from_file__which_have_no_pageids__strbuf, entry.pl_title, entry.pl_title_sz, entries_from_file__which_have_no_pageids);
							if (_title_offset_and_sz != 0){
								const uint32_t _title_offset = _title_offset_and_sz >> 8;
								const uint8_t _title_sz = _title_offset_and_sz & 255;
								wikipedia::page::update_pageid2title_ls(entries_from_file__which_have_no_pageids__strbuf, entry, entries_from_file, _title_offset, _title_sz);
								// printf("%u %.*s\n", (unsigned)entry.pl_pageid, (int)entry.pl_title_sz, entry.pl_title);
							}
						}
						for (EntryIndirect& x : entries_from_file){
							if ((x.pl_title_sz == 0) and (unlikely(x.pl_pageid == entry.pl_pageid))){
								x.pl_title_sz = entry.pl_title_sz;
								memcpy(entries_from_file__which_have_no_pageids__strbuf + entries_from_file__which_have_no_pageids__strbuf_itroffset, entry.pl_title, entry.pl_title_sz);
								x.pl_title_offset = entries_from_file__which_have_no_pageids__strbuf_itroffset;
								entries_from_file__which_have_no_pageids__strbuf_itroffset += entry.pl_title_sz;
								break;
							}
						}
						entry.reset();
					} else if (unlikely(c == ';')){
						reached_end_of_sql_statement = true;
						expecting_next_chars = "\nINSERT INTO `page` VALUES (";
						// errandexit("reached_end_of_sql_statement", contents, i);
					} else if (unlikely(expecting_next_chars != nullptr)){
						if (likely(c == *expecting_next_chars)){
							// NOTE: expecting_next_chars ends with null-byte, so it is safe because c!=0
							++expecting_next_chars;
							if (c == '('){
								reached_end_of_sql_statement = false; // Reached new SQL statement
								expecting_next_chars = nullptr;
								which_field_currently_parsing = 0;
								number_of_single_quotes = 0;
							}
						} else if (c == '/'){
							goto reached_end_of_dump;
						} else {
							return errandexit("Unexpected char when reached_end_of_sql_statement is true", contents, i);
						}
						// End of SQL statement - usually a new one afterwards
					}
					break;
			}
		}
		// fprintf(stderr, "contents_read_into_buf %lu\n", contents_read_into_buf);
		contents_read_into_buf = gzread(fd, contents, buf_sz);
		if (unlikely(contents_read_into_buf == 0))
			break;
	}
	reached_end_of_dump:
	for (const EntryIndirect& entry : entries_from_file){
		if (entry.is_wiki_page){
			if (entry.pl_pageid != 0)
				printf("%u ", entry.pl_pageid);
			else
				printf("\t");
		}
		printf("%.*s\n", (int)entry.pl_title_sz, entries_from_file__which_have_no_pageids__strbuf+entry.pl_title_offset);
	}
	
	if (entries_from_file__which_have_no_pageids__strbuf_sz != 1)
		free(entries_from_file__which_have_no_pageids__strbuf);
	
	return 0;
}
