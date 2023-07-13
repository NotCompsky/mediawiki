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

namespace extract_page_details {
	constexpr std::size_t buf1_sz = 1024*1024 * 10;
	constexpr std::size_t buf_sz = 1024*1024 * 10;
}

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

std::string_view process_file(const int compressed_fd,  char* const output_buf,  const char* searching_for_pageid__as_str,  const off_t init_offset_bytes,  uint32_t* all_citation_urls,  const bool extract_as_html){
	using namespace extract_page_details;
	
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
	
	char* const contents = reinterpret_cast<char*>(malloc(buf_sz));
	
	bz_stream fd;
	memset(&fd, 0, sizeof(fd));
	char* const buf1 = reinterpret_cast<char*>(malloc(buf1_sz));
	if (unlikely(BZ2_bzDecompressInit(&fd, 0, 0 /*int: variable called 'small' for lower-memory decompression*/) != BZ_OK)){
		return "\0ERROR: BZ2_bzDecompressInit\n";
	}
	fd.next_in = buf1;
	fd.avail_in = buf1_sz;
	fd.next_out = contents;
	fd.avail_out = buf_sz;
	
	if (unlikely(lseek(compressed_fd, init_offset_bytes, SEEK_SET) != init_offset_bytes)){
		return "\0ERROR: lseek\n";
	}
	fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
	int bz2_decompress_rc = BZ2_bzDecompress(&fd);
	if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
		return "\0ERROR: BZ2_bzDecompress\n";
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
		for (unsigned i = 0;  i < buf_sz - fd.avail_out;  ++i){
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
							memcpy(output_buf, _start_of_this_page, sz);
							_start_of_this_page = output_buf + sz;
							output_itr = _start_of_this_page;
							char* _itr = output_buf;
							bool processed_this_char;
							bool is_in_pre = false;
							bool is_in_blockquote_sfdsfafsds = false;
							bool is_in_blockquote_sfdsfafsds2= false;
							std::vector<std::string_view> citations;
							bool reflist_found = false;
							while(_itr != _start_of_this_page){
								processed_this_char = false;
								if (
									(_itr[0] == '\n')
								){
									if (_itr[1] == ' '){
										if (not is_in_pre){
											compsky::asciify::asciify(output_itr, "<pre>");
											is_in_pre = true;
										}
									} else if (is_in_pre){
										compsky::asciify::asciify(output_itr, "</pre>");
										is_in_pre = false;
									}
									if (unlikely(is_in_blockquote_sfdsfafsds)){
										output_itr -= 2;
										compsky::asciify::asciify(output_itr, "</blockquote>");
										is_in_blockquote_sfdsfafsds = false;
									} else {
										compsky::asciify::asciify(output_itr, "\n<br>");
									}
									processed_this_char = true;
								}
								if ( // {{cvt|200|ft|m}} or {{convert|200|knots|mph km/h}}
									(_itr[0] == '{') and
									(_itr[1] == '{') and
									(
										(_itr[2] == 'c') or
										(_itr[2] == 'C')
									) and (
										(
											(_itr[3] == 'v') and
											(_itr[4] == 't') and
											(_itr[5] == '|')
										) or (
											(_itr[3] == 'o') and
											(_itr[4] == 'n') and
											(_itr[5] == 'v') and
											(_itr[6] == 'e') and
											(_itr[7] == 'r') and
											(_itr[8] == 't') and
											(_itr[9] == '|')
										)
									)
								){
									char* itr = _itr + 6 + (4*(_itr[3]=='o'));
									char* first_value = itr;
									char* first_unit = nullptr;
									char* first_unit_end = nullptr;
									while((itr != _start_of_this_page) and (*itr != '\n')){
										if (itr[0] == '|'){
											if (first_unit == nullptr)
												first_unit = itr+1;
											else if (first_unit_end == nullptr)
												first_unit_end = itr;
										}
										if ((itr[0] == '}') and (itr[1] == '}'))
											break;
										++itr;
									}
									if (likely((itr[0] == '}') and (itr[1] == '}') and (first_unit_end != nullptr))){
										_itr = itr + 1;
										processed_this_char = true;
										compsky::asciify::asciify(output_itr, "<span class=\"convert-units\">", std::string_view(first_value,compsky::utils::ptrdiff(first_unit-1,first_value)), ' ', std::string_view(first_unit,compsky::utils::ptrdiff(first_unit_end,first_unit)), "</span>");
									}
								}
								if ( // }}
									(_itr[0] == '}') and
									(_itr[1] == '}')
								){
									if (is_in_blockquote_sfdsfafsds2){
										is_in_blockquote_sfdsfafsds2 = false;
										_itr += 1;
										compsky::asciify::asciify(output_itr, "</blockquote>");
										processed_this_char = true;
									}
								}
								if ( // {{reflist}}
									(_itr[0] == '{') and
									(_itr[1] == '{') and
									(
										(_itr[2] == 'r') or
										(_itr[2] == 'R')
									) and
									(_itr[3] == 'e') and
									(_itr[4] == 'f') and
									(_itr[5] == 'l') and
									(_itr[6] == 'i') and
									(_itr[7] == 's') and
									(_itr[8] == 't')
								){
									char* itr = _itr + 9;
									while((itr != _start_of_this_page) and (*itr != '\n')){
										if (
											(itr[0] == '}') and
											(itr[1] == '}')
										)
											break;
										++itr;
									}
									if (likely(
										(itr[0] == '}') and
										(itr[1] == '}')
									)){
										reflist_found = true;
										_itr = itr + 1;
										processed_this_char = true;
										compsky::asciify::asciify(output_itr, "<div id=\"reflist_container\"></div>");
										// reflist is automatically produced at end of file
									}
								}
								if ( // {{quote|
									(_itr[0] == '{') and
									(_itr[1] == '{') and
									(
										(_itr[2] == 'q') and
										(_itr[3] == 'u') and
										(_itr[4] == 'o') and
										(_itr[5] == 't') and
										(_itr[6] == 'e') and
										(_itr[7] == '|')
									) or (
										(
											(_itr[2] == 'b') or
											(_itr[2] == 'B')
										) and
										(_itr[3] == 'l') and
										(_itr[4] == 'o') and
										(_itr[5] == 'c') and
										(_itr[6] == 'k') and
										(_itr[7] == 'q') and
										(_itr[8] == 'u') and
										(_itr[9] == 'o') and
										(_itr[10]== 't') and
										(_itr[11]== 'e') and
										(_itr[12]== '|')
									)
								){
									char* itr = _itr+((_itr[2]=='q')?8:13);
									char* citation_start = itr;
									unsigned n_additional_left_brackets = 0;
									while(itr != _start_of_this_page){
										if (unlikely(itr[0] == '{')){
											++n_additional_left_brackets;
										} else if (unlikely(n_additional_left_brackets != 0) and (itr[0] == '}')){
											--n_additional_left_brackets;
										} else if (
											(itr[0] == '}') and
											(itr[1] == '}')
										)
											break;
										++itr;
									}
									if (likely(
										(itr[0] == '}') and
										(itr[1] == '}')
									)){
										compsky::asciify::asciify(output_itr, "<blockquote>");
										_itr = citation_start - 1;
										is_in_blockquote_sfdsfafsds2 = true;
										processed_this_char = true;
									}
								}
								if (
									(_itr[0] == '{') and
									(_itr[1] == '{') and
									(_itr[2] == 'a') and
									(_itr[3] == 'n') and
									(_itr[4] == 'n') and
									(_itr[5] == 'o') and
									(_itr[6] == 't') and
									(_itr[7] == 'a') and
									(_itr[8] == 't') and
									(_itr[9] == 'e') and
									(_itr[10]== 'd') and
									(_itr[11]== ' ') and
									(_itr[12]== 'l') and
									(_itr[13]== 'i') and
									(_itr[14]== 'n') and
									(_itr[15]== 'k') and
									(_itr[16]== '|')
								){
									char* itr = _itr + 17;
									char* const url_name = itr;
									unsigned n_left_brackets = 1;
									while(itr != _start_of_this_page){
										if (unlikely(itr[0] == '{')){
											++n_left_brackets;
										} else if (itr[0] == '}'){
											--n_left_brackets;
											if (n_left_brackets == 0)
												break;
										}
										++itr;
									}
									if (likely(itr != _start_of_this_page)){
										const std::string_view urlname(url_name,compsky::utils::ptrdiff(itr,url_name));
										compsky::asciify::asciify(output_itr, "<a class=\"wikipage\" href=\"/wiki/", urlname, "\">", urlname, "</a>");
										processed_this_char = true;
										_itr = itr + 1;
									}
								}
								if ( // [http
									(_itr[0] == '[') and
									(_itr[1] == 'h') and
									(_itr[2] == 't') and
									(_itr[3] == 't') and
									(_itr[4] == 'p')
								){
									char* itr = _itr+5;
									if (itr[0] == 's'){
										++itr;
									}
									if (
										(itr[0] == ':') and
										(itr[1] == '/') and
										(itr[2] == '/')
									){
										char* first_space_at = nullptr;
										unsigned n_left_brackets = 1;
										while(itr != _start_of_this_page){
											if (unlikely(itr[0] == '[')){
												++n_left_brackets;
											} else if (itr[0] == ']'){
												--n_left_brackets;
												if (n_left_brackets == 0)
													break;
											} else if ((itr[0] == ' ') and (first_space_at == nullptr)){
												first_space_at = itr;
											}
											++itr;
										}
										if (likely((itr[0] == ']') and (first_space_at != nullptr))){
											citations.emplace_back(_itr+1, compsky::utils::ptrdiff(first_space_at,_itr+1));
											const std::string_view file_type_name(first_space_at+1, compsky::utils::ptrdiff(itr,first_space_at+1));
											compsky::asciify::asciify(output_itr, "<a id=\"citation_use",citations.size(),"\" href=\"#citation",citations.size(),"\">[",file_type_name,"]</a>");
											_itr = itr;
											processed_this_char = true;
										}
									}
								}
								if ( // {{cite {{sfn {{efn
									(
										(_itr[0] == '{') and
										(_itr[1] == '{') and
										(
											(_itr[2] == 's') or
											(_itr[2] == 'S')
										) and
										// ((_itr[2] == 's') or (_itr[2] == 'e')) NOTE: Not processing {{efn because they sometimes contain citations themselves.
										(_itr[3] == 'f') and
										(_itr[4] == 'n') and
										(
											(_itr[5] == '|') or
											(
												(_itr[5] == ' ') and
												(_itr[6] == '|')
											)
										)
									) or ( // {{cite
										(_itr[0] == '{') and
										(_itr[1] == '{') and
										(
											((_itr[2] == 'c') or (_itr[2] == 'C')) and
											(_itr[3] == 'i') and
											(_itr[4] == 't') and
											(
												(
													(_itr[5] == 'e') and
													(
														(_itr[6] == ' ') or
														(_itr[6] == '\n')
													)
												) or (
													(_itr[5] == 'a') and
													(_itr[6] == 't') and
													(_itr[7] == 'i') and
													(_itr[8] == 'o') and
													(_itr[9] == 'n') and
													(
														(_itr[10] == ' ') or
														(_itr[10] == '\n')
													)
												)
											)
										) or (
											(_itr[2] == ' ') and
											
											((_itr[3] == 'c') or (_itr[3] == 'C')) and
											(_itr[4] == 'i') and
											(_itr[5] == 't') and
											(
												(
													(_itr[6] == 'e') and
													(
														(_itr[7] == ' ') or
														(_itr[7] == '\n')
													)
												) or (
													(_itr[6] == 'a') and
													(_itr[7] == 't') and
													(_itr[8] == 'i') and
													(_itr[9] == 'o') and
													(_itr[10]== 'n') and
													(
														(_itr[11] == ' ') or
														(_itr[11] == '\n')
													)
												)
											)
										)
									)
								){
									bool preceded_by_open_ref = false;
									if (
										(_itr[-1] == ';') and
										(_itr[-2] == 't') and
										(_itr[-3] == 'g') and
										(_itr[-4] == '&') and
										(_itr[-5] != '/')
									){
										preceded_by_open_ref = true;
									}
									
									char* itr = _itr;
									if (_itr[5] == 'e') // cite
										itr += 6;
									else if (_itr[2] == 'c') // citation
										itr += 10;
									else // sfn efn
										itr += 6;
									
									char* citation_start = itr;
									unsigned n_additional_left_brackets = 0;
									while(itr != _start_of_this_page){
										if (unlikely(itr[0] == '{')){
											++n_additional_left_brackets;
										} else if (unlikely(n_additional_left_brackets != 0) and (itr[0] == '}')){
											--n_additional_left_brackets;
										} else if (
											(itr[0] == '}') and
											(itr[1] == '}')
										)
											break;
										++itr;
									}
									if (likely(
										(itr[0] == '}') and
										(itr[1] == '}')
									)){
										if (preceded_by_open_ref){
											compsky::asciify::asciify(output_itr, "</ref>");
										}
										
										citations.emplace_back(citation_start, compsky::utils::ptrdiff(itr,citation_start));
										compsky::asciify::asciify(output_itr, "<a id=\"citation_use",citations.size(),"\" href=\"#citation",citations.size(),"\">[",citations.size(),"]</a>");
										_itr = itr + 1;
										processed_this_char = true;
									}
									/*if (
										(_itr[7] == 'n') and
										(_itr[8] == 'e') and
										(_itr[9] == 'w') and
										(_itr[10]== 's') and
										(_itr[11]== '|')
									){
										
									}*/
								}
								if ( // ===[[ ... ]]===
									(_itr[0] == '=') and
									(_itr[1] == '=') and
									(_itr[2] == '=') and
									(_itr[3] == '[') and
									(_itr[4] == '[')
								){
									_itr += 5;
									char* const begin = _itr;
									while(
										((*_itr >= 'a') and (*_itr <= 'z')) or
										((*_itr >= 'A') and (*_itr <= 'Z')) or
										((*_itr >= '0') and (*_itr <= '9')) or
										(*_itr == ' ') or
										(*_itr == '_') or
										(*_itr == '-') or
										(*_itr == '+') or
										(*_itr == '?') or
										(*_itr == '!') or
										(*_itr == ',') or
										(*_itr == '.') or
										(*_itr == ';') or
										(*_itr == ':') or
										(*_itr == '#') or
										(*_itr == ' ')
									){
										++_itr;
									}
									if (likely(
										(_itr[0] == ']') and
										(_itr[1] == ']') and
										(_itr[2] == '=') and
										(_itr[3] == '=') and
										(_itr[4] == '=')
									)){
										compsky::asciify::asciify(output_itr, '<', 'h', '2', '>', std::string_view(begin,compsky::utils::ptrdiff(_itr,begin)), '<', '/', 'h', '2', '>');
										processed_this_char = true;
										_itr += 4;
									} else {
										_itr = begin - 5;
									}
								}
								if ( // == ... ==
									(_itr[-1] == '\n') and
									(_itr[0] == '=') and
									(_itr[1] == '=')
								){
									char* itr = _itr + 2;
									unsigned n_eqls = 2;
									while(*itr == '='){
										++n_eqls;
										++itr;
									}
									char* const begin = itr;
									while(
										(*itr != '\n') and (itr != _start_of_this_page)
									){
										++itr;
									}
									if (likely(
										(itr[-2] == '=') and
										(itr[-1] == '=') and
										(itr[0] == '\n')
									)){
										unsigned n_eqls2 = 2;
										itr -= 3;
										while(*itr == '='){
											++n_eqls2;
											--itr;
										}
										if (n_eqls2 == n_eqls){
											_itr = itr+n_eqls2+1;
											processed_this_char = true;
											const std::string_view title_name(begin,compsky::utils::ptrdiff(itr+1,begin));
											compsky::asciify::asciify(output_itr, "<h",n_eqls," id=\"", title_name , "\">", title_name, "</h",n_eqls,">");
										}
									}
								}
								if (
									(_itr[0] == ':') and
									(_itr[1] == '\'') and
									(_itr[2] == '\'')
								){
									char* itr2 = _itr;
									while((*itr2 != '\n') and (itr2 != _start_of_this_page))
										++itr2;
									if (likely(
										(itr2[-1] == '\'') and
										(itr2[-2] == '\'')
									)){
										compsky::asciify::asciify(output_itr, "<blockquote>");
										is_in_blockquote_sfdsfafsds = true;
										_itr += 2;
										processed_this_char = true;
									}
								}
								if ( // ''' ... '''
									(_itr[0] == '\'') and
									(_itr[1] == '\'') and
									(_itr[2] == '\'') and
									(_itr[3] != '\'')
								){
									_itr += 3;
									char* const begin = _itr;
									while(
										((*_itr >= 'a') and (*_itr <= 'z')) or
										((*_itr >= 'A') and (*_itr <= 'Z')) or
										((*_itr >= '0') and (*_itr <= '9')) or
										(*_itr == ' ') or
										(*_itr == '_') or
										(*_itr == '-') or
										(*_itr == '+') or
										(*_itr == '?') or
										(*_itr == '!') or
										(*_itr == ',') or
										(*_itr == '.') or
										(*_itr == ';') or
										(*_itr == ':') or
										(*_itr == '#') or
										(*_itr == ' ')
									){
										++_itr;
									}
									if (likely(
										(_itr[0] == '\'') and
										(_itr[1] == '\'') and
										(_itr[2] == '\'')
									)){
										compsky::asciify::asciify(output_itr, '<', 'e', 'm', '>', std::string_view(begin,compsky::utils::ptrdiff(_itr,begin)), '<', '/', 'e', 'm', '>');
										processed_this_char = true;
										_itr += 2;
									} else {
										_itr = begin - 3;
									}
								}
								if ((_itr[0] == '[') and (_itr[1] == '[')){
									
								}
								if (_itr[0] == '&'){
									if ( // &amp;
										(_itr[1] == 'a') and
										(_itr[2] == 'm') and
										(_itr[3] == 'p') and
										(_itr[4] == ';')
									){
										compsky::asciify::asciify(output_itr, '&');
										processed_this_char = true;
										_itr += 4;
									} else if ( // &lt;
										(_itr[1] == 'l') and
										(_itr[2] == 't') and
										(_itr[3] == ';')
									){
										// Wikipedia pages often use self-closing <ref name="foobar"/> tags, which is now forbidden by HTML5.
										// TODO: Currently <ref name=":2"/> -> <ref name=":2&quot; "></ref> // TODO: id reference 9jsoijfdsoijdfwefs
										if (
											(_itr[4] == 'r') and
											(_itr[5] == 'e') and
											(_itr[6] == 'f') and
											(_itr[7] == ' ') and
											(_itr[8] == 'n') and
											(_itr[9] == 'a') and
											(_itr[10]== 'm') and
											(_itr[11]== 'e') and
											(_itr[12]== '=')
										){
											const bool is_quoted = (
												(_itr[13]== '&') and
												(_itr[14]== 'q') and
												(_itr[15]== 'u') and
												(_itr[16]== 'o') and
												(_itr[17]== 't') and
												(_itr[18]== ';')
											);
											
											//printf("()() %.50s\n", _itr);
											
											char* name_end = _itr+((is_quoted)?18:12);
											//printf(" nm  %.50s\n", name_end);
											while((name_end != _start_of_this_page) and (*name_end != '&') and (*name_end != '\n'))
												++name_end;
											//printf("     %.50s\n", name_end);
											if (likely(name_end[0] == '&')){
												if (is_quoted){
													if (
														(name_end[1] == 'q') and
														(name_end[2] == 'u') and
														(name_end[3] == 'o') and
														(name_end[4] == 't') and
														(name_end[5] == ';')
													){
														name_end += 6;
														if (name_end[0] == ' '){
															++name_end;
														}
													}
												}
												char* itr = name_end;
												//printf(" itr %.50s\n", itr);
												
												bool is_good = false;
												if (is_quoted){
													if (
														(itr[0] == '/') and
														(itr[1] == '&') and
														(itr[2] == 'g') and
														(itr[3] == 't') and
														(itr[4] == ';')
													){
														itr += 4;
														is_good = true;
													}
												} else {
													if (
														((itr-1)[0] == '/') and
														(itr[0] == '&') and
														(itr[1] == 'g') and
														(itr[2] == 't') and
														(itr[3] == ';')
													){
														itr += 3;
														is_good = true;
													}
												}
												if (likely(is_good)){
													//printf("--GOOD--\nitr = %.20s\n", itr);
													compsky::asciify::asciify(output_itr, "<ref name=\"", std::string_view(_itr+19,compsky::utils::ptrdiff(name_end,_itr+19)),"\"></ref>");
													processed_this_char = true;
													_itr = itr;
												}
											}
										}
										if (not processed_this_char){
											compsky::asciify::asciify(output_itr, '<');
											processed_this_char = true;
											_itr += 3;
										}
									} else if ( // &gt;
										(_itr[1] == 'g') and
										(_itr[2] == 't') and
										(_itr[3] == ';')
									){
										compsky::asciify::asciify(output_itr, '>');
										processed_this_char = true;
										_itr += 3;
									} else if ( // &quot;
										(_itr[1] == 'q') and
										(_itr[2] == 'u') and
										(_itr[3] == 'o') and
										(_itr[4] == 't') and
										(_itr[5] == ';')
									){
										compsky::asciify::asciify(output_itr, '"');
										processed_this_char = true;
										_itr += 5;
									} else if ( // &ndash;
										(_itr[1] == 'n') and
										(_itr[2] == 'd') and
										(_itr[3] == 'a') and
										(_itr[4] == 's') and
										(_itr[5] == 'h') and
										(_itr[6] == ';')
									){
										compsky::asciify::asciify(output_itr, '-','-','-');
										processed_this_char = true;
										_itr += 6;
									} else if ( // &nbsp;
										(_itr[1] == 'n') and
										(_itr[2] == 'b') and
										(_itr[3] == 's') and
										(_itr[4] == 'p') and
										(_itr[5] == ';')
									){
										compsky::asciify::asciify(output_itr, ' ');
										processed_this_char = true;
										_itr += 5;
									} else {
										fprintf(stderr, "extract-page.hpp: Possibly unknown &escape; _itr = %.10s\n", _itr);
									}
								}
								if ( // {{Main|
									(_itr[0] == '{') and
									(_itr[1] == '{') and
									(
										(_itr[2] == 'M') or
										(_itr[2] == 'm')
									) and
									(_itr[3] == 'a') and
									(_itr[4] == 'i') and
									(_itr[5] == 'n') and
									(_itr[6] == '|')
								){
									char* const name_start = _itr + 7;
									char* itr = name_start;
									while((itr != _start_of_this_page) and (*itr != '\n')){
										if (
											(itr[0] == '}') and
											(itr[1] == '}')
										){
											break;
										}
										++itr;
									}
									if (likely(itr != _start_of_this_page)){
										const std::string_view title(name_start,compsky::utils::ptrdiff(itr,name_start));
										compsky::asciify::asciify(output_itr, "<a class=\"wikipage\" href=\"/wiki/",title,"\">Main: ",title,"</a>");
										processed_this_char = true;
										_itr = itr + 1;
									}
								}
								if ( // [[...]]
									(_itr[0] == '[') and
									(_itr[1] == '[')
								){
									_itr += 2;
									char* const begin = _itr;
									char* url_name = nullptr;
									unsigned n_left_brackets = 2;
									while(
										/*(*_itr == '|') or
										(*_itr == '(') or
										(*_itr == ')') or
										(*_itr == '/') or
										(*_itr == '\'') or
										(*_itr == '"') or
										(*_itr == '&') or
										
										((*_itr >= 'a') and (*_itr <= 'z')) or
										((*_itr >= 'A') and (*_itr <= 'Z')) or
										((*_itr >= '0') and (*_itr <= '9')) or
										(*_itr == ' ') or
										(*_itr == '_') or
										(*_itr == '-') or
										(*_itr == '+') or
										(*_itr == '?') or
										(*_itr == '!') or
										(*_itr == ',') or
										(*_itr == '.') or
										(*_itr == ';') or
										(*_itr == ':') or
										(*_itr == '#') or
										(*_itr == ' ')*/
										(_itr != _start_of_this_page) and (*_itr != '\n')
									){
										if ((*_itr == '|') and (likely(url_name == nullptr))){
											url_name = _itr+1;
										}
										if (*_itr == '[')
											++n_left_brackets;
										if (*_itr == ']'){
											--n_left_brackets;
											if (n_left_brackets == 0)
												break;
										}
										++_itr;
									}
									char* urlslug_end = _itr;
									if (url_name != nullptr){
										urlslug_end = url_name;
									} else {
										url_name = begin;
									}
									const std::string_view urlslug(begin,compsky::utils::ptrdiff(urlslug_end,begin)-1);
									const std::string_view urlname(url_name,compsky::utils::ptrdiff(_itr,url_name)-1);
									if (likely(n_left_brackets == 0)){
										compsky::asciify::asciify(output_itr, "<a class=\"wikipage\" href=\"/wiki/", urlslug, "\">", urlname, "</a>");
										processed_this_char = true;
									} else {
										_itr = begin - 2;
									}
								}
								if (not processed_this_char){
									compsky::asciify::asciify(output_itr, *_itr);
								}
								++_itr;
							}
							
							if (unlikely(not reflist_found)){
								compsky::asciify::asciify(output_itr, "<div id=\"reflist_container\"></div>");
							}
							
							// do citations:
							compsky::asciify::asciify(output_itr, "<div class=\"citation_ref\" id=\"reflist\">");
							
							unsigned all_citation_urls__indx = 0;
							for (unsigned i = 0;  i < citations.size();  ++i){
								const std::string_view& sv = citations[i];
								const char* const sv_data = sv.data();
								compsky::asciify::asciify(output_itr, "<div id=\"citation",i+1,"\"><a href=\"#citation_use",i+1,"\">[",i+1,"]</a> ");
								const unsigned buf_offset = compsky::utils::ptrdiff(output_itr,_start_of_this_page);
								unsigned url_start_ = 0;
								for (unsigned url_start = 0;  url_start < sv.size()-4;  ++url_start){
									if (
										((sv_data-1)[url_start] != '-') and
										(sv_data[url_start  ] == 'u') and
										(sv_data[url_start+1] == 'r') and
										(sv_data[url_start+2] == 'l') and
										(sv_data[url_start+3] == '=')
									){
										url_start_ = url_start+4;
										break;
									}
								}
								if (url_start_ != 0){
									unsigned url_end_;
									for (url_end_ = url_start_;  url_end_ < sv.size();  ++url_end_){
										if ((sv[url_end_] == ' ') or (sv[url_end_] == '|'))
											break;
									}
									if (all_citation_urls != nullptr){
										all_citation_urls[all_citation_urls__indx++] = buf_offset+url_start_;
										all_citation_urls[all_citation_urls__indx++] = buf_offset+url_end_;
										all_citation_urls[all_citation_urls__indx++] = i;
									}
								}
								compsky::asciify::asciify(output_itr, sv, "</div>");
							}
							if (all_citation_urls != nullptr){
								all_citation_urls[all_citation_urls__indx++] = 0;
								all_citation_urls[all_citation_urls__indx++] = 0;
								all_citation_urls[all_citation_urls__indx++] = 0;
							}
							compsky::asciify::asciify(output_itr, "</div>");
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
		fd.avail_out = buf_sz;
		fd.next_out = contents;
		if (fd.avail_in == 0){
			fd.next_in = buf1;
			fd.avail_in = read(compressed_fd, fd.next_in, buf1_sz);
			if (unlikely(fd.avail_in == 0)){
				return "\0ERROR: Reached end of file?\n";
			}
		}
		bz2_decompress_rc = BZ2_bzDecompress(&fd);
		if (unlikely((bz2_decompress_rc != BZ_OK) and (bz2_decompress_rc != BZ_STREAM_END))){
			return "\0ERROR: BZ2_bzDecompress\n";
		}
	}
	
	finish_extraction:
	
	BZ2_bzDecompressEnd(&fd);
	
	return std::string_view(_start_of_this_page, _start_of_this_page_sz);
}
