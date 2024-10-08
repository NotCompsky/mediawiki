#include "extract-page.hpp"
#include "get-byte-offset-of-page-given-title.hpp"

static char title_str_buf[1+255+1];
constexpr std::size_t buf_sz = 1024*1024;
static char* const gz_contents_buf = reinterpret_cast<char*>(malloc(buf_sz + get_byte_offset_of_page_given_title::max_line_sz*2));
static char* const html_output_buf = reinterpret_cast<char*>(malloc(1024*1024*10));

extern "C"
void extract_page__init(){
	pages_articles_multistream_index_txt_offsetted_gz__init();
}
extern "C"
void extract_page__deinit(){
	pages_articles_multistream_index_txt_offsetted_gz__deinit();
}

extern "C"
int open_file(const char* const filepath){
	return open(filepath, O_RDONLY);
}

extern "C"
uint64_t extract_page_given_title(const int archive_fd,  const int archiveindices_fd,  char* const output_buf,  uint32_t* const all_citation_urls,  const char* const title_requested,  const int is_wikipedia){
	using namespace compsky_wiki_extractor;
	
	/*{
		constexpr std::string_view _msg("extract_page_given_title: ");
		char _buf[_msg.size()+255+1];
		memcpy(_buf, _msg.data(), _msg.size());
		memcpy(_buf+_msg.size(), title_requested, strlen(title_requested));
		_buf[_msg.size()+strlen(title_requested)] = '\n';
		write(2, _buf, _msg.size()+strlen(title_requested)+1);
	}
	*/
	const OffsetAndPageid offset_and_pageid(get_byte_offset_and_pageid_given_title(archiveindices_fd, title_requested, gz_contents_buf,buf_sz, title_str_buf, is_wikipedia));
	if (unlikely(offset_and_pageid.pageid == nullptr))
		return 0;
	const std::string_view errstr(process_file(archive_fd, html_output_buf, offset_and_pageid.pageid, offset_and_pageid.offset, all_citation_urls, true));
	if (unlikely(errstr.data()[0] == '\0')){
		write(2, errstr.data(), errstr.size());
		return 0;
	}
	memcpy(output_buf, errstr.data(), errstr.size());
	return errstr.size();
}

extern "C"
uint64_t extract_raw_page_given_title(const int archive_fd,  const int archiveindices_fd,  char* const output_buf,  uint32_t* const all_citation_urls,  const char* const title_requested,  const int is_wikipedia){
	using namespace compsky_wiki_extractor;
	
	const OffsetAndPageid offset_and_pageid(get_byte_offset_and_pageid_given_title(archiveindices_fd, title_requested, gz_contents_buf,buf_sz, title_str_buf, is_wikipedia));
	if (unlikely(offset_and_pageid.pageid == nullptr))
		return 0;
	const std::string_view errstr(process_file(archive_fd, html_output_buf, offset_and_pageid.pageid, offset_and_pageid.offset, all_citation_urls, false));
	if (unlikely(errstr.data()[0] == '\0')){
		write(2, errstr.data(), errstr.size());
		return 0;
	}
	memcpy(output_buf, errstr.data(), errstr.size());
	return errstr.size();
}



#include "read_wikimedia-page-quickmetadata.hpp"

extern "C"
uint64_t get_all_pages_of_interest_as_html(char* const output_buf,  const char* const filename){
	using namespace compsky_wiki_extractor;
	
	std::vector<wikipedia::page::EntryIndirect> entries_from_file;
	std::vector<wikipedia::page::StringView> entries_from_file__which_have_no_pageids;
	std::vector<wikipedia::page::FileToOffset> entries_from_file__file_indx_offsets;
	char* entries_from_file__which_have_no_pageids__strbuf = output_buf + 1024*1024;
	std::size_t entries_from_file__which_have_no_pageids__strbuf_sz = 1;
	unsigned entries_from_file__which_have_no_pageids__strbuf_itroffset = 0;
	
	entries_from_file__file_indx_offsets.emplace_back(entries_from_file.size(), filename);
	const char* errstr2;
	const char* const errstr1 = parse::parse_pageid2title_file(
		errstr2,
		filename,
		entries_from_file__which_have_no_pageids__strbuf,
		entries_from_file__which_have_no_pageids__strbuf_sz,
		entries_from_file,
		entries_from_file__which_have_no_pageids
	);
	if (likely(errstr1 == nullptr)){
		char* itr = output_buf;
		for (const wikipedia::page::EntryIndirect& entry : entries_from_file){
			const std::string_view name(entries_from_file__which_have_no_pageids__strbuf+entry.pl_title_offset, entry.pl_title_sz);
			if (entry.is_wiki_page){
				compsky::asciify::asciify(itr, "<li><a class=\"wikipage\" href=\"/wiki/",name,"\">",name,"</a></li>");
			} else {
				compsky::asciify::asciify(itr, "<li>",name,"</li>");
			}
		}
		return compsky::utils::ptrdiff(itr, output_buf);
	} else {
		std::size_t nnn = strlen(errstr1);
		memcpy(output_buf, errstr1, nnn);
		if (errstr2 != nullptr){
			memcpy(output_buf+nnn, errstr2, strlen(errstr2));
			nnn += strlen(errstr2);
		}
		return nnn;
	}
}
