#ifndef NO_IMPORT_STD_VECTOR
# include <vector>
#endif
#ifndef NO_IMPORT_STD_STRINGVIEW
# include <string_view>
#endif
#ifndef NO_IMPORT_COMPSKY_ASCIIFY
# include <compsky/asciify/asciify.hpp>
#endif


namespace compsky_wiki_extractor {

#ifdef WASM_TYPEDEFS
typedef unsigned size_t;
typedef unsigned uintptr_t;
#endif

size_t ptrdiff(const void* p,  const void* q){
	// WARNING: Assumes p >= q  - hence why the type is not ptrdiff_t
	return reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(q);
}

#ifdef NO_IMPORT_STD_STRINGVIEW
struct StringView {
	const char* _data;
	size_t _size;
	const char* data() const {
		return this->_data;
	}
	size_t size() const {
		return this->_size;
	}
	StringView(
		const char* const a,
		const size_t b
	)
	: _data(a)
	, _size(b)
	{}
	char operator[](const unsigned indx) const {
		return this->_data[indx];
	}
};
#else
typedef std::string_view StringView;
#endif


#ifdef NO_IMPORT_COMPSKY_ASCIIFY
namespace compsky {
namespace asciify {
void asciify(char*& itr);
template<typename... Args>
void asciify(char*& itr,  const char* str,  Args&&... args);
template<typename... Args>
void asciify(char*& itr,  const StringView& str,  Args&&... args);
template<typename... Args>
void asciify(char*& itr,  const char c,  Args&&... args);

void asciify(char*& itr){}
template<typename... Args>
void asciify(char*& itr,  const char* str,  Args&&... args){
	while(*str != 0){
		*itr = *str;
		++itr;
		++str;
	}
	asciify(itr, args...);
}
template<typename... Args>
void asciify(char*& itr,  const StringView& str,  Args&&... args){
	for (unsigned i = 0;  i < str.size();  ++i){
		*itr = str[i];
		++itr;
	}
	asciify(itr, args...);
}
template<typename... Args>
void asciify(char*& itr,  const char c,  Args&&... args){
	*itr = c;
	++itr;
	asciify(itr, args...);
}

}
}
#endif


#ifdef NO_IMPORT_STD_VECTOR
void maybevector__emplace_back(StringView*& citations,  const size_t citations_size,    char* const str_begin,  const size_t str_len){
	citations[citations_size]._data = str_begin;
	citations[citations_size]._size = str_len;
	--citations;
}
size_t maybevector__size(StringView* const citations,  const size_t citations_size){
	return citations_size;
}
void initialise_array(StringView*& citations,  char* const output_buf_end){
	citations = reinterpret_cast<StringView*>(output_buf_end);
	--citations;
}
#else
void maybevector__emplace_back(std::vector<StringView>& citations,  const std::size_t citations_size,  char* const str_begin,  const std::size_t str_len){
	citations.emplace_back(str_begin, str_len);
}
std::size_t maybevector__size(std::vector<StringView>& citations,  const std::size_t citations_size){
	return citations.size();
}
void initialise_array(std::vector<StringView>& citations,  char* const output_buf_end){}
#endif

template<typename T>
char* wikitext_to_html(
	char* const _start_of_this_page
	, char* const output_buf
#ifdef NO_IMPORT_STD_VECTOR
	, char* const output_buf_end
#endif
#ifndef NO_WIKITEXT_CITATION_ARRAY
	, uint32_t* const all_citation_urls
#endif
){
							char* output_itr = _start_of_this_page;
							char* _itr = output_buf;
							bool processed_this_char;
							bool is_in_pre = false;
							bool is_in_blockquote_sfdsfafsds = false;
							bool is_in_blockquote_sfdsfafsds2= false;
							T citations;
#ifdef NO_IMPORT_STD_VECTOR
							initialise_array(citations, output_buf_end);
#endif
							size_t citations_indx = 0;
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
									if (is_in_blockquote_sfdsfafsds){
										[[unlikely]]
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
									if ((itr[0] == '}') and (itr[1] == '}') and (first_unit_end != nullptr)){
										[[likely]]
										_itr = itr + 1;
										processed_this_char = true;
										compsky::asciify::asciify(output_itr, "<span class=\"convert-units\">", StringView(first_value,ptrdiff(first_unit-1,first_value)), ' ', StringView(first_unit,ptrdiff(first_unit_end,first_unit)), "</span>");
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
									if (
										(itr[0] == '}') and
										(itr[1] == '}')
									){
										[[likely]]
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
										if (itr[0] == '{'){
											[[unlikely]]
											++n_additional_left_brackets;
										} else if ((n_additional_left_brackets != 0) and (itr[0] == '}')){
											[[unlikely]]
											--n_additional_left_brackets;
										} else if (
											(itr[0] == '}') and
											(itr[1] == '}')
										)
											break;
										++itr;
									}
									if (
										(itr[0] == '}') and
										(itr[1] == '}')
									){
										[[likely]]
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
										if (itr[0] == '{'){
											[[unlikely]]
											++n_left_brackets;
										} else if (itr[0] == '}'){
											--n_left_brackets;
											if (n_left_brackets == 0)
												break;
										}
										++itr;
									}
									if (itr != _start_of_this_page){
										[[likely]];
										const StringView urlname(url_name,ptrdiff(itr,url_name));
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
											if (itr[0] == '['){
												[[unlikely]]
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
										if ((itr[0] == ']') and (first_space_at != nullptr)){
											[[likely]]
											maybevector__emplace_back(citations, citations_indx, _itr+1, ptrdiff(first_space_at,_itr+1));
											++citations_indx;
											const StringView file_type_name(first_space_at+1, ptrdiff(itr,first_space_at+1));
											compsky::asciify::asciify(output_itr, "<a id=\"citation_use",maybevector__size(citations, citations_indx),"\" href=\"#citation",maybevector__size(citations, citations_indx),"\">[",file_type_name,"]</a>");
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
										if (itr[0] == '{'){
											[[unlikely]]
											++n_additional_left_brackets;
										} else if ((n_additional_left_brackets != 0) and (itr[0] == '}')){
											[[unlikely]]
											--n_additional_left_brackets;
										} else if (
											(itr[0] == '}') and
											(itr[1] == '}')
										)
											break;
										++itr;
									}
									if (
										(itr[0] == '}') and
										(itr[1] == '}')
									){
										[[likely]]
										if (preceded_by_open_ref){
											compsky::asciify::asciify(output_itr, "</ref>");
										}
										
										maybevector__emplace_back(citations, citations_indx, citation_start, ptrdiff(itr,citation_start));
										++citations_indx;
										compsky::asciify::asciify(output_itr, "<a id=\"citation_use",maybevector__size(citations, citations_indx),"\" href=\"#citation",maybevector__size(citations, citations_indx),"\">[",maybevector__size(citations, citations_indx),"]</a>");
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
									if (
										(_itr[0] == ']') and
										(_itr[1] == ']') and
										(_itr[2] == '=') and
										(_itr[3] == '=') and
										(_itr[4] == '=')
									){
										[[likely]]
										compsky::asciify::asciify(output_itr, '<', 'h', '2', '>', StringView(begin,ptrdiff(_itr,begin)), '<', '/', 'h', '2', '>');
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
									if (
										(itr[-2] == '=') and
										(itr[-1] == '=') and
										(itr[0] == '\n')
									){
										[[likely]];
										unsigned n_eqls2 = 2;
										itr -= 3;
										while(*itr == '='){
											++n_eqls2;
											--itr;
										}
										if (n_eqls2 == n_eqls){
											_itr = itr+n_eqls2+1;
											processed_this_char = true;
											const StringView title_name(begin,ptrdiff(itr+1,begin));
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
									if (
										(itr2[-1] == '\'') and
										(itr2[-2] == '\'')
									){
										[[likely]]
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
									if (
										(_itr[0] == '\'') and
										(_itr[1] == '\'') and
										(_itr[2] == '\'')
									){
										[[likely]]
										compsky::asciify::asciify(output_itr, '<', 'e', 'm', '>', StringView(begin,ptrdiff(_itr,begin)), '<', '/', 'e', 'm', '>');
										processed_this_char = true;
										_itr += 2;
									} else {
										_itr = begin - 3;
									}
								}
								if ((_itr[0] == '{') and (_itr[1] == '|') and (_itr[2] == 'c') and (_itr[3] == 'l') and (_itr[4] == 'a') and (_itr[5] == 's') and (_itr[6] == 's') and (_itr[7] == '=') and (_itr[8] == '"') and (_itr[9] == 'w') and (_itr[10] == 'i') and (_itr[11] == 'k') and (_itr[12] == 'i') and (_itr[13] == 't') and (_itr[14] == 'a') and (_itr[15] == 'b') and (_itr[16] == 'l') and (_itr[17] == 'e')){
									// wikitable
									// TODO
									char* const begin = _itr + 18;
									char* itr = _itr;
									while((itr != _start_of_this_page) and ((itr[0] != '|') or (itr[1] != '}')))
										++itr;
									if ((itr[0] == '|') and (itr[1] == '}')){
										[[likely]]
										compsky::asciify::asciify(output_itr, "<table>");
										char* const end = itr;
										itr = begin;
										bool opened_tr = false;
										while (itr != end){
											if ((itr[0] == '|') and (itr[1] == '-') and (itr[2] == '\n')){
												if (opened_tr)
													compsky::asciify::asciify(output_itr, "</tr>");
												compsky::asciify::asciify(output_itr, "<tr>");
												opened_tr = true;
												itr += 3;
											} else if ((itr[0] == '\n') and (itr[1] == '!')){
												const char* row_start = itr+1;
												itr += 2;
												while((itr != end) and (itr[0] != '|') and (itr[0] != '\n')){
													++itr;
												}
												if (itr[0] == '|'){
													[[likely]]
													compsky::asciify::asciify(output_itr, "<th ",StringView(row_start,ptrdiff(itr,row_start)),">");
												}
											} else {
												itr += 1;
											}
										}
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
											if (name_end[0] == '&'){
												[[likely]]
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
												if (is_good){
													[[likely]]
													//printf("--GOOD--\nitr = %.20s\n", itr);
													compsky::asciify::asciify(output_itr, "<ref name=\"", StringView(_itr+19,ptrdiff(name_end,_itr+19)),"\"></ref>");
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
#ifdef PRINT_ERRORS
										fprintf(stderr, "extract-page.hpp: Possibly unknown &escape; _itr = %.10s\n", _itr);
#endif
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
									if (itr != _start_of_this_page){
										[[likely]];
										const StringView title(name_start,ptrdiff(itr,name_start));
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
										if ((*_itr == '|') and (url_name == nullptr)){
											[[likely]]
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
									const StringView urlslug(begin,ptrdiff(urlslug_end,begin)-1);
									const StringView urlname(url_name,ptrdiff(_itr,url_name)-1);
									if (n_left_brackets == 0){
										[[likely]]
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
							
							if (not reflist_found){
								[[unlikely]]
								compsky::asciify::asciify(output_itr, "<div id=\"reflist_container\"></div>");
							}
							
							// do citations:
							compsky::asciify::asciify(output_itr, "<div class=\"citation_ref\" id=\"reflist\">");
							
							unsigned all_citation_urls__indx = 0;
							for (unsigned i = 0;  i < maybevector__size(citations, citations_indx);  ++i){
								const StringView& sv = citations[i];
								const char* const sv_data = sv.data();
								compsky::asciify::asciify(output_itr, "<div id=\"citation",i+1,"\"><a href=\"#citation_use",i+1,"\">[",i+1,"]</a> ");
								const unsigned buf_offset = ptrdiff(output_itr,_start_of_this_page);
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
#ifndef NO_WIKITEXT_CITATION_ARRAY
									if (all_citation_urls != nullptr){
										all_citation_urls[all_citation_urls__indx++] = buf_offset+url_start_;
										all_citation_urls[all_citation_urls__indx++] = buf_offset+url_end_;
										all_citation_urls[all_citation_urls__indx++] = i;
									}
#endif
								}
								compsky::asciify::asciify(output_itr, sv, "</div>");
							}
#ifndef NO_WIKITEXT_CITATION_ARRAY
							if (all_citation_urls != nullptr){
								all_citation_urls[all_citation_urls__indx++] = 0;
								all_citation_urls[all_citation_urls__indx++] = 0;
								all_citation_urls[all_citation_urls__indx++] = 0;
							}
#endif
							compsky::asciify::asciify(output_itr, "</div>");
}
}
