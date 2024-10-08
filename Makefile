default: ~/bin/wiki_getquickpagemetadata2 ~/bin/wiki_pagelinks ~/bin/wiki_extract-page-given-id ~/bin/wiki_extract-page-given-title ~/bin/wiki_get-byte-offset-given-title ~/lib/extract-page.so

~/bin/wiki_getquickpagemetadata2:
	c++ -std=c++20 -O3 read_wikimedia-page-quickmetadata.cpp -o ~/bin/wiki_getquickpagemetadata2 -lbz2 -lz

~/bin/wiki_pagelinks:
	c++ -std=c++20 -O3 read_wikimedia-pagelinks-sql.cpp -o ~/bin/wiki_pagelinks -lz -march=native

read_wikimedia-pagelinks-sql--old:
	c++ -std=c++20 -O3 read_wikimedia-pagelinks-sql--old.cpp -o read_wikimedia-pagelinks-sql--old

~/bin/wiki_extract-page-given-id:
	c++ -std=c++20 -O3 extract-page.cpp -o ~/bin/wiki_extract-page-given-id -lbz2

~/bin/wiki_extract-page-given-title:
	c++ -std=c++20 -O3 extract-page-given-title.cpp -o ~/bin/wiki_extract-page-given-title -lbz2 -lz

~/bin/wiki_get-byte-offset-given-title:
	c++ -std=c++20 -O3 get-byte-offset-of-page-given-title.cpp -o ~/bin/wiki_get-byte-offset-given-title -lz

~/lib/extract-page.so:
	c++ -std=c++20 -O3 extract-page-lib.cpp -o ~/lib/extract-page.so -lbz2 -lz -fPIC -shared -march=native

wikitext2html.wasm:
	clang++-15 -mbulk-memory '-DNO_SIMD' '-DRPILL_IS_FOR_PUBLIC_TESTING=${RPILL_IS_FOR_PUBLIC_TESTING}' -fno-delete-null-pointer-checks -Wall --std=c++20 --target=wasm32 -O3 -I/usr/local/lib --no-standard-libraries "-Wl,--initial-memory=10944512" -Wl,--export=wikitext2html -Wl,--no-entry -o wikitext2html.wasm wikitext_to_html.cpp -DNO_IMPORT_STD_VECTOR -DNO_IMPORT_STD_STRINGVIEW -DNO_WIKITEXT_CITATION_ARRAY -DNO_IMPORT_COMPSKY_ASCIIFY -DWASM_TYPEDEFS
