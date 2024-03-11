#include "wikitext_to_html.hpp"

extern "C"
void wikitext2html(char* const content,  const unsigned content_len){
	compsky_wiki_extractor::wikitext_to_html<compsky_wiki_extractor::StringView*>(content, content, content+content_len);
}
