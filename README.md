# Description

Render Wikipedia pages from a compressed archive.

Wikipedia's source code is written in MediaWiki - which is notoriously complex. There are very few libraries that render MediaWiki pages - the existing ones rely on a complex render pipeline that can involve numerous network requests (slow).

This, written in C++, is very fast. But it lacks most features (it doesn't even render tables). It is designed for a bare-bones web server for hosting Wikipedia articles.
