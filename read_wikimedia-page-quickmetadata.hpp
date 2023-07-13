#pragma once

#include <vector>
#include <compsky/utils/ptrdiff.hpp>
#include <compsky/os/read.hpp>


namespace wikipedia {
namespace page {

struct Entry {
	uint32_t pl_pageid;
	int8_t pl_namespace;
	uint8_t pl_title_sz;
	char pl_title[255];
	void reset(){
		this->pl_pageid = 0;
		this->pl_namespace = 0;
		this->pl_title_sz = 0;
	}
};
struct EntryIndirect {
	uint32_t pl_pageid;
	uint8_t pl_title_sz;
	bool is_wiki_page;
	uint32_t pl_title_offset;
	EntryIndirect(const bool _is_wiki_page,  uint32_t _pageid,  char* _buf_start,  char* _title_start,  char* _title_end_plus1)
	: pl_pageid(_pageid)
	, pl_title_sz(compsky::utils::ptrdiff(_title_end_plus1,_title_start))
	, is_wiki_page(_is_wiki_page)
	, pl_title_offset(compsky::utils::ptrdiff(_title_start, _buf_start))
	{}
	EntryIndirect(const bool _is_wiki_page,  uint32_t _pageid,  uint32_t _title_offset,  uint8_t _title_sz)
	: pl_pageid(_pageid)
	, pl_title_sz(_title_sz)
	, is_wiki_page(_is_wiki_page)
	, pl_title_offset(_title_offset)
	{}
};
struct StringView {
	const uint32_t buf_offset;
	const uint8_t sz;
	StringView(char* const _buf_start,  char* const _content_start,  char* const _end_plus1)
	: buf_offset(compsky::utils::ptrdiff(_content_start,_buf_start))
	, sz(compsky::utils::ptrdiff(_end_plus1,_content_start))
	{}
	StringView(const uint32_t _offset,  const uint32_t _sz)
	: buf_offset(_offset)
	, sz(_sz)
	{}
};
struct FileToOffset {
	const unsigned indx_offset;
	const char* const filepath;
	FileToOffset(const unsigned _indx,  const char* const _filepath)
	: indx_offset(_indx)
	, filepath(_filepath)
	{}
};

int errandexit(const char* const errstr, const char* const buf,  const unsigned i){
	fprintf(stderr, "ERROR: %s\n", errstr);
	fprintf(stderr, "       Previous 300 chars: %.100s\n", buf+((i>300)?i:300)-300);
	fprintf(stderr, "       This char: %c\n", buf[i]);
	fprintf(stderr, "       Next     100 chars: %.100s\n", buf+i);
	return 1;
}

uint64_t in_list(const char* const buf_start,  const char* const title,  const std::size_t title_sz,  const std::vector<StringView>& ls){
	for (const StringView& x : ls){
		if (x.sz == title_sz){
			bool matches = true;
			for (unsigned i = 0;  i < title_sz;  ++i){
				matches &= (buf_start[x.buf_offset+i] == title[i]);
			}
			if (unlikely(matches))
				return (x.buf_offset << 8) | x.sz;
		}
	}
	return 0;
}

void update_pageid2title_ls(const char* const buf_start,  const uint32_t entry_pageid,  std::vector<EntryIndirect>& entries_from_file,  const uint32_t title_offset,  const uint8_t title_sz){
	for (EntryIndirect& x : entries_from_file){
		if (x.pl_title_sz == title_sz){
			bool matches = true;
			for (unsigned i = 0;  i < title_sz;  ++i){
				matches &= (buf_start[x.pl_title_offset+i] == buf_start[title_offset+i]);
			}
			if (matches){
				if (unlikely(x.pl_pageid == entry_pageid)){
					return;
				} else if (x.pl_pageid == 0){
					x.pl_pageid = entry_pageid;
					return;
				}
			}
		}/* else if ((x.pl_title_sz == 0) and (unlikely(x.pl_pageid == entry_pageid))){
			x.pl_title_sz = title_sz;
			x.pl_title_offset = title_offset;
			return;
		}*/
	}
	entries_from_file.emplace_back(true, entry_pageid, title_offset, title_sz);
}

}
}

namespace parse {

const char* parse_pageid2title_file(
	const char*& errstr2,
	const char* const filename,
	char*& entries_from_file__which_have_no_pageids__strbuf,
	std::size_t& entries_from_file__which_have_no_pageids__strbuf_sz,
	std::vector<wikipedia::page::EntryIndirect>& entries_from_file,
	std::vector<wikipedia::page::StringView>& entries_from_file__which_have_no_pageids
){
	using namespace wikipedia::page;
	
	static char filepathbuf[74+50+1] = "/home/vangelic/bin/backupable_from_repos/wikipedia/interesting-wiki-pages/";
	char* _itr = filepathbuf;
	memcpy(_itr+74, filename, strlen(filename)+1);
	compsky::os::ReadOnlyFile f(filepathbuf);
	if (unlikely(f.is_null())){
		errstr2 = filepathbuf;
		return "Cannot open file: ";
	}
	const std::size_t f_sz = f.size<0>();
	char* const _buffer = reinterpret_cast<char*>(malloc(entries_from_file__which_have_no_pageids__strbuf_sz+f_sz+2));
	if (unlikely(_buffer == 0)){
		return "Cannot allocate memory";
	}
	if (entries_from_file__which_have_no_pageids__strbuf_sz == 1){
		_buffer[0] = '\0'; // Empty the byte, which exists to avoid segfault when doing itr[-1]
	} else {
		memcpy(entries_from_file__which_have_no_pageids__strbuf, _buffer, entries_from_file__which_have_no_pageids__strbuf_sz);
		free(entries_from_file__which_have_no_pageids__strbuf);
	}
	entries_from_file__which_have_no_pageids__strbuf = _buffer;
	char* _buf = _buffer + entries_from_file__which_have_no_pageids__strbuf_sz;
	f.read_entirety_into_buf(_buf);
	_buf[f_sz+0] = '\n';
	_buf[f_sz+1] = '\0';
	entries_from_file__which_have_no_pageids__strbuf_sz += f_sz+2;
	char* itr = _buf;
	while(*itr != 0){
		if (*itr == '\t'){
		} else if (*itr == '#'){
			char* const _begin = itr;
			while(*itr != '\n')
				++itr;
			entries_from_file.emplace_back(false, 0, entries_from_file__which_have_no_pageids__strbuf, _begin, itr);
		} else if (((*itr >= '0') and (*itr <= '9')) and not (itr[-1] == '\t')){
			uint32_t pageid = 0;
			while((*itr >= '0') and (*itr <= '9')){
				pageid *= 10;
				pageid += *itr - '0';
				++itr;
			}
			if (unlikely((*itr != ' ') and (*itr != '\n'))){
				char* _start = itr;
				while ((_start != _buf) and (*_start != '\n'))
					--_start;
				errstr2 = _start+1;
				while(*itr != '\n')
					++itr;
				*itr = 0;
				return "Bad file format: Missing SPACE: ";
			}
			char* new_entry_title = itr;
			if (*itr == ' '){ // else it has an empty title
				++new_entry_title;
				++itr;
				while(*itr != '\n')
					++itr;
			}
			entries_from_file.emplace_back(true, pageid, entries_from_file__which_have_no_pageids__strbuf, new_entry_title, itr);
			entries_from_file__which_have_no_pageids.emplace_back(entries_from_file__which_have_no_pageids__strbuf, new_entry_title, itr);
		} else if (*itr == '\n'){
			entries_from_file.emplace_back(false, 0, entries_from_file__which_have_no_pageids__strbuf, itr, itr+1);
		} else if ((itr[-1] == '\t') or (itr[-1] == '\n')){
			char* new_entry_title = itr;
			while(*itr != '\n')
				++itr;
			entries_from_file.emplace_back(true, 0, entries_from_file__which_have_no_pageids__strbuf, new_entry_title, itr);
			entries_from_file__which_have_no_pageids.emplace_back(entries_from_file__which_have_no_pageids__strbuf, new_entry_title, itr);
		} else {
			printf(">>>%c<<<\n",  itr[0]);
			printf(">>>%u<<<\n", +itr[0]);
			itr[100] = 0;
			errstr2 = itr;
			return "Bad file format: ";
		}
		++itr;
	}
	return nullptr;
}

};
