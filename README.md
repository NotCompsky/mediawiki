# Description

tl;dr: Ugly but readable. Very fast, available in WASM (unlike all alternatives, which are server-side).

Render Wikipedia pages from a compressed archive.

Wikipedia's source code is written in MediaWiki - which is notoriously complex. There are very few libraries that render MediaWiki pages - the existing ones rely on a complex render pipeline that can involve numerous network requests (slow).

This, written in C++, is very fast. But it lacks most features (it doesn't even render tables). It is designed for a bare-bones web server for hosting Wikipedia articles.

![mediawiki1](https://github.com/user-attachments/assets/5c5b8df1-b51b-406a-828a-dce1389110f7)

![mediawiki2](https://github.com/user-attachments/assets/9ff51f9e-897b-4ae1-ba7d-55cabb505462)
