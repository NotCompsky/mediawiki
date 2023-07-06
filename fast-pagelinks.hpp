// c++ -std=c++20 fast-pagelinks.cpp -o /tmp/fast-pagelinks -O3 -march=native

#ifdef __AVX2__
# include <x86intrin.h> // Required for SIMD instructions on x86 architectures
#endif
#include <cstdint> // for std::size_t
#include <cstdlib> // for malloc
#include <cstring> // for memcpy


#include <cstdio> // for printf


void* aligned_malloc(const std::size_t required_bytes,  const std::size_t alignment){
	void* orig_ptr;
	void** p2; // aligned block
	int offset = alignment - 1 + sizeof(void*);
	if ((orig_ptr = (void*)malloc(required_bytes + offset)) == nullptr){
		return nullptr;
	}
	p2 = (void**)(((size_t)(orig_ptr) + offset) & ~(alignment - 1));
	p2[-1] = orig_ptr;
	return p2;
}


struct PageIDOrName1 {
	union {
		char as_name[32];
		uint16_t as_pageid[16];
	};
	unsigned compare(const char* const _str) const {
#ifdef __AVX2__
		__m256i this_ = _mm256_load_si256((const __m256i*)this->as_name);
		__m256i othr_ = _mm256_load_si256((const __m256i*)_str);
		
		 __m256i pcmp = _mm256_cmpeq_epi16(this_, othr_);
		return _mm256_movemask_epi8(pcmp);
#else
		static_assert(false, "No AVX2");
#endif
	}
	unsigned compare__unaligned(const char* const _str) const {
#ifdef __AVX2__
		__m256i this_ = _mm256_load_si256((const __m256i*)this->as_name);
		__m256i othr_ = _mm256_loadu_si256((const __m256i*)_str);
		
		 __m256i pcmp = _mm256_cmpeq_epi16(this_, othr_);
		return _mm256_movemask_epi8(pcmp);
#else
		static_assert(false, "No AVX2");
#endif
	}
	bool is_name_same(const char* const _str) const {
		unsigned rc = this->compare(_str);
		// printf("is_name_same %s %s: %u\n", this->as_name, _str, rc);
		return (rc == 0xffffffffU);
	}
	bool is_name_same__unaligned(const char* const _str) const {
		return (this->compare__unaligned(_str) == 0xffffffffU);
	}
	bool is_name_empty() const {
		constexpr const char zeros[32] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
		return (this->compare__unaligned(zeros) == 0xffffffffU);
	}
	void set_name(const char* const _str){
		memcpy(this->as_name, _str, 32);
	}
	bool contains_pageid(const uint16_t _pageid) const {
		__m256i vb = _mm256_setr_epi16(
			_pageid,_pageid,_pageid,_pageid,
			_pageid,_pageid,_pageid,_pageid,
			_pageid,_pageid,_pageid,_pageid,
			_pageid,_pageid,_pageid,_pageid
		);
		__m256i othr_ = _mm256_loadu_si256((const __m256i*)this->as_pageid);
		
		__m256i pcmp = _mm256_cmpeq_epi16(vb, othr_);
		
		const unsigned rc = _mm256_movemask_epi8(pcmp);
		return (rc != 0x00000000U);
	}
};
#ifdef  AVX512F
struct PageIDOrName2 {
	union {
		char as_name[64];
		uint16_t as_pageid[32];
	};
	bool is_name_same(const char* const _str){
		__m512i this_ = _mm512_load_si512((__m512i*)this->as_name);
		__m512i othr_ = _mm512_load_si512((__m512i*)_str);
		
		 __m512i pcmp = _mm512_cmpeq_epi32(this_, othr_);
		return (_mm512_movepi8_mask(pcmp) == 0xffffffffffffffffU);
	}
	void set_name(const char* const _str){
		memcpy(this->as_name, _str, 64);
	}
};
#endif




#ifdef OLD_FORMAT2
struct PageIDOrName {
	/* Name if last byte is 0, pageid if last bye is 255 (meaning comparisons will always succeed on the last bit between names, and always fail for pageids)
	The intended use is to have an array of this struct like so:
	PAGEID
		NAME (of connected page)
		NAME
		...
		NAME
	PAGEID
		NAME
		...
	Where each element can be compared against a string - and if the comparison fails at the last byte, it means it is a pageid */
	union {
		uint32_t as_pageid[8];
		char as_name[32];
	};
	unsigned compare(const PageIDOrName& other){
#ifdef __AVX2__
		__m256i this_ = _mm256_load_si256((__m256i*)this->as_name);
		__m256i othr_ = _mm256_load_si256((__m256i*)other.as_name);
		
		 __m256i pcmp = _mm256_cmpeq_epi16(this_, othr_);
		return _mm256_movemask_epi8(pcmp);
#else
		static_assert(false, "No AVX2");
#endif
	}
	bool are_equal(const PageIDOrName& other){
		return (this->compare(other) == 0xffffffffU);
	}
};
#endif


#ifdef OLD_FORMAT


struct ThirtyTwoBytes {
	uint32_t from_pageid;
	char name[28]; // NOTE: Wiki page names can be up to 255 chars
};


struct TinyLink {
	union {
		ThirtyTwoBytes as_types;
		std::uint8_t as_bytes[32];
	} data;
	void set__assuming_already_zerod(const uint32_t _from_pageid,  const char* const _name_start,  const unsigned _name_len){
		this->data.as_types.from_pageid = _from_pageid;
		memcpy(this->data.as_types.name, _name_start, (_name_len>28)?28:_name_len);
	}
	bool compare(const TinyLink& other){
#ifdef __AVX2__
		__m256i this_ = _mm256_load_si256((__m256i*)this->data.as_bytes);
		__m256i othr_ = _mm256_load_si256((__m256i*)other.data.as_bytes);
		
		 __m256i pcmp = _mm256_cmpeq_epi16(this_, othr_);
		unsigned bitmask = _mm256_movemask_epi8(pcmp);
		return (bitmask == 0xffffffffU);
#else
		static_assert(false, "No AVX2");
#endif
	}
	bool compare_names(const TinyLink& other){
#ifdef __AVX2__
		__m256i this_ = _mm256_load_si256((__m256i*)this->data.as_bytes);
		__m256i othr_ = _mm256_load_si256((__m256i*)other.data.as_bytes);
		
		 __m256i pcmp = _mm256_cmpeq_epi16(this_, othr_);
		unsigned bitmask = _mm256_movemask_epi8(pcmp);
		return ((bitmask & 0xfffffff0U) == 0xfffffff0U);
#else
		static_assert(false, "No AVX2");
#endif
	}
};

#endif
