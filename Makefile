default: ~/bin/wiki_page-sql ~/bin/wiki_pagelinks ~/bin/wiki_extract-page-given-id ~/bin/wiki_extract-page-given-title ~/bin/wiki_get-byte-offset-given-title

~/bin/wiki_page-sql:
	c++ -std=c++20 -O3 read_wikimedia-page-sql.cpp -o ~/bin/wiki_page-sql -lbz2 -lz

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
