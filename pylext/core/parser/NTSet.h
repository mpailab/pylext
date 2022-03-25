#pragma once
#include <immintrin.h>
#include <unordered_set>
#include "Exception.h"
struct NTSetS { // Множество нетерминалов
    static constexpr int max_value = ~(1 << (sizeof(int)*8-1));
	std::unordered_set<int> s;
	using iterator = std::unordered_set<int>::const_iterator;
	using value_type = int;
	[[nodiscard]] bool intersects(const NTSetS& y) const {
		for (int x : y.s)
			if (s.count(x))return true;
		return false;
	}
	int intersects(const NTSetS& y, int* B) const {
		int res = 0;
		for (int x : y.s)
			if (s.count(x)) {
				res++;
				*B = x;
				if (res >= 2)return res;
			}
		return res;
	}
	/// Фильтрация множества нетерминалов
	bool filter(NTSetS& y)const {
		for (auto it = y.s.begin(); it != y.s.end();) {
			if (s.count(*it))++it;
			else it = y.s.erase(it);
		}
		return !y.s.empty();
	}
	[[nodiscard]] bool has(int x)const {
		return s.count(x) > 0;
	}
	bool add(int x) {
		return s.insert(x).second;
	}
	bool add_check(int x) {
		return s.insert(x).second;
	}
	[[nodiscard]] bool empty()const { return s.empty(); }
	NTSetS& clear() { s.clear(); return *this; }
	NTSetS() = default;
	NTSetS(const std::initializer_list<int>& l) :s(l) {}
	NTSetS& operator|=(const NTSetS& ss) {
		for (int i : ss.s)
			add(i);
		return *this;
	}
	NTSetS& operator&=(const NTSetS& ns) {
		for (auto it = s.begin(); it != s.end(); )
			if (!ns.has(*it))it = s.erase(it);
			else ++it;
		return *this;
	}
	NTSetS operator|(const NTSetS& x) const {
		NTSetS r(*this);
		return r |= x;
	}
	NTSetS operator&(const NTSetS& x) const {
		NTSetS r(*this);
		return r &= x;
	}
	NTSetS& operator=(const std::initializer_list<int>& l) {
		s = l;
		return *this;
	}
	NTSetS& operator=(const vector<int>& l) {
		s.clear();
		s.insert(l.begin(), l.end());
		return *this;
	}
	[[nodiscard]] iterator begin()const { return s.begin(); }
	[[nodiscard]] iterator end()const { return s.end(); }
};

struct NTSetV {
    static constexpr int max_value = ~(1 << (sizeof(int)*8-1));
    vector<uint64_t> mask;
	struct iterator  {
		using iterator_category = std::forward_iterator_tag;
		using value_type = int;
		using difference_type = int;
		using reference = int;
		using pointer = int *;
		uint64_t curr;
		int pos = 0;
		const uint64_t* ptr, * end;
		iterator(const uint64_t* p, size_t l) :curr(0), ptr(p), end(p + l) {
			next();
		}
		iterator& next() {
			while (ptr < end && !*ptr) {
				pos += 64;
				++ptr;
			}
			if (ptr != end)
				curr = *ptr;
			return *this;
		}
		iterator& operator++() {
			if (curr &= (curr - 1))return *this;
			++ptr; pos += 64;
			return next();
		}
		int operator*() const {
			return pos + (int)_tzcnt_u64(curr);
		}
		bool operator==(const iterator& i)const {
			return ptr == i.ptr && curr == i.curr;
		}
		bool operator!=(const iterator& i)const {
			return ptr != i.ptr || curr != i.curr;
		}
	};
	[[nodiscard]] bool intersects(const NTSetV& y) const {
		for (int i = 0, sz = (int)std::min(y.mask.size(),mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i])return true;
		return false;
	}
	int intersects(const NTSetV& y, int *B) const {
		int m = 0;
		for (size_t i = 0, sz = std::min(y.mask.size(), mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i]) {
				*B = int((i << 6u) + _tzcnt_u64(mask[i] & y.mask[i]));
				m += (int)_mm_popcnt_u64(mask[i] & y.mask[i]);
				if (m >= 2)return 2;
			}
		return m;
	}
	/// Фильтрация множества нетерминалов
	bool filter(NTSetV& y)const {
		//y &= *this;
		int sz = (int)min(y.mask.size(), mask.size());
		y.mask.resize(sz);
		uint64_t r = 0;
		for (int i = 0; i < sz; i++)
			r |= y.mask[i] &= mask[i];
		return r != 0;
	}
	[[nodiscard]] bool has(unsigned x)const {
		return (x >> 6u) < mask.size() && ((mask[x >> 6u] >> (x & 63u)) & 1u) != 0;
	}
	bool add_check(int x) {
		if (has(x))return false;
		add(x);
		return true;
	}
	void add(unsigned x) {
		unsigned y = x >> 6u;
		if (y >= mask.size())
			mask.resize(y + 1);
		mask[y] |= 1ULL << (x & 63u);
	}
	[[nodiscard]] int size()const {
		int r = 0;
		for (auto x : mask)
			r += (int)_mm_popcnt_u64(x);
		return r;
	}
	[[nodiscard]] bool empty()const {
		for (auto x : mask)
			if (x)return false;
		return true;
	}
	NTSetV& clear() { fill(mask.begin(), mask.end(), 0); return *this; }
	NTSetV() = default;
	NTSetV(std::initializer_list<int>& l) { for (int i : l)add(i); }
	NTSetV& operator|=(const NTSetV& s) {
		mask.resize(max(mask.size(), s.mask.size()), 0ULL);
		for (int i = 0, sz = (int)s.mask.size(); i < sz; i++)
			mask[i] |= s.mask[i];
		return *this;
	}
	NTSetV& operator&=(const NTSetV& ns) {
		ns.filter(*this);
		return *this;
	}
	NTSetV operator|(const NTSetV& x) const {
		NTSetV r(*this);
		return r |= x;
	}
	NTSetV operator&(const NTSetV& x) const {
		NTSetV r(*this);
		return r &= x;
	}
	NTSetV& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	[[nodiscard]] iterator begin()const { return iterator(mask.data(), mask.size()); }
	[[nodiscard]] iterator end()const { return iterator(mask.data() + mask.size(), 0); }
};

inline uint64_t extract64(__m256i x, unsigned i) {
	switch (i) {
	case 0: return _mm256_extract_epi64(x, 0);
	case 1: return _mm256_extract_epi64(x, 1);
	case 2: return _mm256_extract_epi64(x, 2);
	case 3: return _mm256_extract_epi64(x, 3);
	default: return 0;
	}
}

//#define HAS_AVX512
#ifdef HAS_AVX512
#include <zmmintrin.h>

inline __m512i bitmask512(int i) {
	return _mm512_mask_set1_epi64(_mm512_setzero_si512(), 1 << (i >> 6), 1ULL << (i & 63));
}

inline __m256i bitmask256(int i) {
	return _mm256_mask_set1_epi64(_mm256_setzero_si256(), 1 << (i >> 6), 1ULL << (i & 63));
}

inline __m128i bitmask128(int i) {
	return _mm_mask_set1_epi64(_mm_setzero_si128(), 1 << (i >> 6), 1ULL << (i & 63));
}

struct NTSetV8 { // “ребует поддержки AVX-512
	__m512i x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask;
		uint32_t pmask;
		int pos = 0;
		__m512i x;
		iterator() : cmask(0), pmask(0) {}
		iterator(__m512i p) :x(p) {
			pmask = _mm512_test_epi64_mask(x,x);
			cmask = _mm512_mask_reduce_or_epi64(_blsi_u32(pmask), x);
			pos = _tzcnt_u32(pmask) << 6;
		}
		iterator& next() {
			pmask = _blsr_u32(pmask);
			cmask = _mm512_mask_reduce_or_epi64(_blsi_u32(pmask), x);
			pos = _tzcnt_u32(pmask) << 6;
			return *this;
		}
		iterator& operator++() {
			if ((cmask = _blsr_u64(cmask)))return *this;
			return next();
		}
		int operator*() const {
			return (int)_tzcnt_u64(cmask) + pos;
		}
		bool operator==(const iterator& i)const {
			return cmask == i.cmask && pmask == i.pmask;
		}
		bool operator!=(const iterator& i)const {
			return cmask != i.cmask || pmask != i.pmask;
		}
		bool atEnd() {
			return !pmask;
		}
		bool last() {
			return !_blsr_u64(cmask) && !_blsr_u64(pmask);
		}
	};
	bool empty() const {
		return !_mm512_test_epi64_mask(x,x);
	}
	bool intersects(const NTSetV8& y) const {
		return _mm512_test_epi64_mask(x, y.x) != 0;
	}
	int intersects(const NTSetV8& y, int* B) const {
		iterator it(_mm512_and_si512(x, y.x));
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// ‘ильтраци¤ множества нетерминалов
	bool filter(NTSetV8& y)const {
		y.x = _mm512_and_si512(x, y.x);
		return !y.empty();
	}
	bool has(int i)const {
		return _mm512_mask_test_epi64_mask(1 << (i >> 6), x, _mm512_set1_epi64(1ULL << (i & 63)))!=0;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return _mm512_cmpneq_epi64_mask(y, x)!=0;
	}
	void add(int i) {
		x = _mm512_mask_or_epi64(x, (1<<(i>>6)), x, _mm512_set1_epi64(1ULL<<(i&63)));
	}
	int size()const {
		//int m = _mm512_test_epi64_mask(x);
		union {
			uint64_t t[8];
			__m512 x;
		} r;
		_mm512_store_epi64(r.t, x);
		return (int)_mm512_reduce_add_epi64(_mm512_setr_epi64(_mm_popcnt_u64(r.t[0]), _mm_popcnt_u64(r.t[1]), _mm_popcnt_u64(r.t[2]), _mm_popcnt_u64(r.t[3]), _mm_popcnt_u64(r.t[4]), _mm_popcnt_u64(r.t[5]), _mm_popcnt_u64(r.t[6]),_mm_popcnt_u64(r.t[7])));
	}
	NTSetV8& clear() { x = _mm512_setzero_si512(); return *this; }
	NTSetV8() :x(_mm512_setzero_si512()) {};
	NTSetV8(__m512i v) :x(v) {};
	NTSetV8(std::initializer_list<int>& l) : x(_mm512_setzero_si512()) { for (int i : l)add(i); }
	NTSetV8& operator|=(const NTSetV8& s) {
		x = _mm512_or_si512(x, s.x);
		return *this;
	}
	NTSetV8& operator&=(const NTSetV8& s) {
		x = _mm512_and_si512(x, s.x);
		return *this;
	}
	NTSetV8 operator|(const NTSetV8& s) const {
		return _mm512_or_si512(x, s.x);
	}
	NTSetV8 operator&(const NTSetV8& s) const {
		return _mm512_and_si512(x, s.x);
	}
	NTSetV8& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV8& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	iterator begin()const { return iterator(x); }
	iterator end()const { return iterator(); }
};


struct NTSetV4 {
	__m256i x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask;
		uint32_t pmask;
		__m256i x;
		iterator() : cmask(0), pmask(0) {}
		iterator(__m256i p) :x(p) {
			pmask = _mm256_test_epi64_mask(x, x);// _mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(x, _mm256_setzero_si256()))) ^ 0xF;
			cmask = pmask ? extract64(x, _tzcnt_u32(pmask)) : 0;
		}
		iterator& next() {
			pmask = _blsr_u32(pmask);
			cmask = pmask ? extract64(x, _tzcnt_u32(pmask)) : 0;
			return *this;
		}
		iterator& operator++() {
			if ((cmask = _blsr_u64(cmask)))return *this;
			return next();
		}
		int operator*() const {
			return (int)_tzcnt_u64(cmask) + _tzcnt_u32(pmask) * 64;
		}
		int* operator->() const {
			return 0;
		}
		bool operator==(const iterator& i)const {
			return cmask == i.cmask && pmask == i.pmask;
		}
		bool operator!=(const iterator& i)const {
			return cmask != i.cmask || pmask != i.pmask;
		}
		bool atEnd() {
			return !pmask;
		}
		bool last() {
			return !(cmask & (cmask - 1)) && !(pmask & (pmask - 1));
		}
	};
	bool empty() const {
		return _mm256_testz_si256(x, x) != 0;
	}
	bool intersects(const NTSetV4& y) const {
		return !_mm256_testz_si256(x, y.x);
	}
	int intersects(const NTSetV4& y, int* B) const {
		iterator it(_mm256_and_si256(x, y.x));
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// ‘ильтраци¤ множества нетерминалов
	bool filter(NTSetV4& y)const {
		y.x = _mm256_and_si256(x, y.x);
		return !y.empty();
	}
	bool has(int i)const {
		return _mm256_mask_test_epi64_mask(1 << (i >> 6), x, _mm256_set1_epi64x(1ULL << (i & 63))) != 0;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return _mm256_cmpneq_epi64_mask(y, x) != 0;
	}
	void add(int i) {
		x = _mm256_mask_or_epi64(x, (1 << (i >> 6)), x, _mm256_set1_epi64x(1ULL << (i & 63)));
	}
	int size()const {
		return int(_mm_popcnt_u64(_mm256_extract_epi64(x, 0)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 1)) +
			_mm_popcnt_u64(_mm256_extract_epi64(x, 2)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 3)));
	}
	NTSetV4& clear() { x = _mm256_setzero_si256(); return *this; }
	NTSetV4() :x(_mm256_setzero_si256()) {};
	NTSetV4(__m256i v) :x(v) {};
	NTSetV4(std::initializer_list<int>& l) : x(_mm256_setzero_si256()) { for (int i : l)add(i); }
	NTSetV4& operator|=(const NTSetV4& s) {
		x = _mm256_or_si256(x, s.x);
		return *this;
	}
	NTSetV4& operator&=(const NTSetV4& s) {
		x = _mm256_and_si256(x, s.x);
		return *this;
	}
	NTSetV4 operator|(const NTSetV4& s) const {
		return _mm256_or_si256(x, s.x);
	}
	NTSetV4 operator&(const NTSetV4& s) const {
		return _mm256_and_si256(x, s.x);
	}
	NTSetV4& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV4& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	iterator begin()const { return iterator(x); }
	iterator end()const { return iterator(); }
};

struct NTSetV2 {
	__m128i x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask[3];
		uint32_t pos;
		//__m128i x;
		iterator() : pos(2) {}
		iterator(__m128i p) /*:x(p)*/ {
			cmask[0] = _mm_extract_epi64(p, 0);
			cmask[1] = _mm_extract_epi64(p, 1);
			cmask[2] = 0;
			pos = cmask[0] ? 0 : cmask[1] ? 1 : 2;
		}
		iterator& operator++() {
			if (cmask[pos] &= (cmask[pos] - 1))return *this;
			else if (++pos < 2 && !cmask[pos])pos = 2;
			return *this;
		}
		int operator*() const {
			return (int)_tzcnt_u64(cmask[pos]) + pos * 64;
		}
		int* operator->() const {
			return 0;
		}
		bool operator==(const iterator& i)const {
			return pos == i.pos && cmask[pos] == i.cmask[pos];
		}
		bool operator!=(const iterator& i)const {
			return pos != i.pos && cmask[pos] != i.cmask[pos];
		}
		bool atEnd() {
			return pos >= 2;
		}
		bool last() {
			return pos < 2 && !cmask[pos + 1] && !(cmask[pos] & (cmask[pos] - 1));
		}
	};
	bool empty() const {
		return _mm_testz_si128(x, x) != 0;
	}
	bool intersects(const NTSetV2& y) const {
		return !_mm_testz_si128(x, y.x);
	}
	int intersects(const NTSetV2& y, int* B) const {
		iterator it(_mm_and_si128(x, y.x));
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// ‘ильтраци¤ множества нетерминалов
	bool filter(NTSetV2& y)const {
		y.x = _mm_and_si128(x, y.x);
		return !y.empty();
	}
	bool has(int i)const {
		return (((i & 64 ? _mm_extract_epi64(x, 1) : _mm_extract_epi64(x, 0)) >> (i & 63)) & 1) != 0;
		//return int(_mm_extract_epi16(x, i >> 4) >> (i & 15)) & 1;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return !_mm_testc_si128(y, x);
	}
	void add(int i) {
		x = _mm_or_si128(x, bitmask128(i));
	}
	int size()const {
		return int(_mm_popcnt_u64(_mm_extract_epi64(x, 0)) + _mm_popcnt_u64(_mm_extract_epi64(x, 1)));
	}
	NTSetV2& clear() { x = _mm_setzero_si128(); return *this; }
	NTSetV2() :x(_mm_setzero_si128()) {};
	NTSetV2(__m128i v) :x(v) {};
	NTSetV2(std::initializer_list<int>& l) : x(_mm_setzero_si128()) { for (int i : l)add(i); }
	NTSetV2& operator|=(const NTSetV2& s) {
		x = _mm_or_si128(x, s.x);
		return *this;
	}
	NTSetV2& operator&=(const NTSetV2& s) {
		x = _mm_and_si128(x, s.x);
		return *this;
	}
	NTSetV2 operator|(const NTSetV2& s) const {
		return _mm_or_si128(x, s.x);
	}
	NTSetV2 operator&(const NTSetV2& s) const {
		return _mm_and_si128(x, s.x);
	}
	NTSetV2& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV2& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	iterator begin()const { return iterator(x); }
	iterator end()const { return iterator(); }
};

#else

inline __m256i bitmask256(unsigned i) {
	return _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0), _mm256_set1_epi32(i >> 5)), _mm256_set1_epi32(1U << (i & 31u)));
}
inline __m128i bitmask128(unsigned i) {
	return i & 64u ? _mm_set_epi64x((1ULL << (i & 63u)), 0) : _mm_set_epi64x(0, 1ULL << i);
}

struct NTSetV4 {
    static constexpr int max_value = 255;
    __m256i x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask;
		uint32_t pmask;
		__m256i x{};
		iterator() : cmask(0), pmask(0) {}
        explicit iterator(__m256i p) :x(p) {
			pmask = (uint32_t)_mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(x, _mm256_setzero_si256())))^0xFu;
			cmask = pmask ? extract64(x, _tzcnt_u32(pmask)) : 0;
		}
		iterator& next() {
			pmask &= (pmask - 1);
			cmask = pmask ? extract64(x, _tzcnt_u32(pmask)) : 0;
			return *this;
		}
		iterator& operator++() {
			if (cmask &= (cmask - 1))return *this;
			return next();
		}
		int operator*() const {
			return (int)_tzcnt_u64(cmask) + (int)_tzcnt_u32(pmask)*64;
		}
		int* operator->() const {
			return 0;
		}
		bool operator==(const iterator& i)const {
			return cmask == i.cmask && pmask == i.pmask;
		}
		bool operator!=(const iterator& i)const {
			return cmask != i.cmask || pmask != i.pmask;
		}
		[[nodiscard]] bool atEnd() const {
			return !pmask;
		}
		[[nodiscard]] bool last() const {
			return !(cmask & (cmask - 1)) && !(pmask & (pmask - 1));
		}
	};
	[[nodiscard]] bool empty() const {
		return _mm256_testz_si256(x, x)!=0;
	}
	[[nodiscard]] bool intersects(const NTSetV4& y) const {
		return !_mm256_testz_si256(x, y.x);
	}
	int intersects(const NTSetV4& y, int* B) const {
		iterator it(_mm256_and_si256(x, y.x));
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// фильтрация множества нетерминалов
	bool filter(NTSetV4& y)const {
		y.x = _mm256_and_si256(x, y.x);
		return !y.empty();
	}
	[[nodiscard]] int has(unsigned i)const {
		return int((extract64(x, i >> 6u) >> (i&63u)) & 1u);
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return !_mm256_testc_si256(y,x);
	}
	void add(int i) {
		x = _mm256_or_si256(x, bitmask256(i));
	}
	[[nodiscard]] int size()const {
		return int(_mm_popcnt_u64(_mm256_extract_epi64(x, 0)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 1)) + 
			_mm_popcnt_u64(_mm256_extract_epi64(x, 2)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 3)));
	}
	NTSetV4& clear() { x=_mm256_setzero_si256(); return *this; }
	NTSetV4() :x(_mm256_setzero_si256()) {};
	explicit NTSetV4(__m256i v) :x(v) {};
	NTSetV4(std::initializer_list<int>& l): x(_mm256_setzero_si256()) { for (int i : l)add(i); }
	NTSetV4& operator|=(const NTSetV4& s) {
		x = _mm256_or_si256(x, s.x);
		return *this;
	}
	NTSetV4& operator&=(const NTSetV4& s) {
		x = _mm256_and_si256(x, s.x);
		return *this;
	}
	NTSetV4 operator|(const NTSetV4& s) const {
		return NTSetV4(_mm256_or_si256(x, s.x));
	}
	NTSetV4 operator&(const NTSetV4& s) const {
		return NTSetV4(_mm256_and_si256(x, s.x));
	}
	NTSetV4& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV4& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	[[nodiscard]] iterator begin()const { return iterator(x); }
	[[nodiscard]] iterator end()const { return iterator(); }
};

struct NTSetV2 {
    static constexpr int max_value = 127;
	__m128i x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask[3];
		uint32_t pos;
		//__m128i x;
		iterator() : pos(2) {}
		explicit iterator(__m128i p) /*:x(p)*/ {
			cmask[0] = _mm_extract_epi64(p, 0);
			cmask[1] = _mm_extract_epi64(p, 1);
			cmask[2] = 0;
			pos = cmask[0] ? 0 : cmask[1] ? 1 : 2;
		}
		iterator& operator++() {
			if (cmask[pos] &= (cmask[pos] - 1))return *this;
			else if(++pos<2&&!cmask[pos])pos=2;
			return *this;
		}
		int operator*() const {
			return int(_tzcnt_u64(cmask[pos]) + pos * 64);
		}
		int* operator->() const {
			return 0;
		}
		bool operator==(const iterator& i)const {
			return pos == i.pos && cmask[pos] == i.cmask[pos];
		}
		bool operator!=(const iterator& i)const {
			return pos != i.pos && cmask[pos] != i.cmask[pos];
		}
		[[nodiscard]] bool atEnd() const {
			return pos>=2;
		}
		[[nodiscard]] bool last() const {
			return pos<2 && !cmask[pos+1] && !(cmask[pos] & (cmask[pos] - 1));
		}
	};
	[[nodiscard]] bool empty() const {
		return _mm_testz_si128(x, x) != 0;
	}
	[[nodiscard]] bool intersects(const NTSetV2& y) const {
		return !_mm_testz_si128(x, y.x);
	}
	int intersects(const NTSetV2& y, int* B) const {
		iterator it(_mm_and_si128(x, y.x));
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// Фильтрация множества нетерминалов
	bool filter(NTSetV2& y)const {
		y.x = _mm_and_si128(x, y.x);
		return !y.empty();
	}
	[[nodiscard]] bool has(int i)const {
		return (((i & 64 ? _mm_extract_epi64(x, 1) : _mm_extract_epi64(x, 0)) >> (i & 63)) & 1) != 0;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return !_mm_testc_si128(y, x);
	}
	void add(int i) {
		x = _mm_or_si128(x, bitmask128(i));
	}
	[[nodiscard]] int size()const {
		return int(_mm_popcnt_u64(_mm_extract_epi64(x, 0)) + _mm_popcnt_u64(_mm_extract_epi64(x, 1)));
	}
	NTSetV2& clear() { x = _mm_setzero_si128(); return *this; }
	NTSetV2() :x(_mm_setzero_si128()) {};
	explicit NTSetV2(__m128i v) :x(v) {};
	NTSetV2(std::initializer_list<int>& l) : x(_mm_setzero_si128()) { for (int i : l)add(i); }
	NTSetV2& operator|=(const NTSetV2& s) {
		x = _mm_or_si128(x, s.x);
		return *this;
	}
	NTSetV2& operator&=(const NTSetV2& s) {
		x = _mm_and_si128(x, s.x);
		return *this;
	}
	NTSetV2 operator|(const NTSetV2& s) const {
		return NTSetV2(_mm_or_si128(x, s.x));
	}
	NTSetV2 operator&(const NTSetV2& s) const {
		return NTSetV2(_mm_and_si128(x, s.x));
	}
	NTSetV2& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV2& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	[[nodiscard]] iterator begin()const { return iterator(x); }
	[[nodiscard]] iterator end()const { return iterator(); }
};
#endif

struct NTSetV1 {
    static constexpr int max_value = 63;
	uint64_t x;
	struct iterator {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		uint64_t cmask;
		//__m128i x;
		explicit iterator(uint64_t p = 0) :cmask(p){}
		iterator& operator++() {
			cmask = _blsr_u64(cmask);
			return *this;
		}
		int operator*() const {
			return (int)_tzcnt_u64(cmask);
		}
		bool operator==(const iterator& i)const {
			return cmask == i.cmask;
		}
		bool operator!=(const iterator& i)const {
			return cmask!=i.cmask;
		}
		[[nodiscard]] bool atEnd() const {
			return !cmask;
		}
		[[nodiscard]] bool last() const {
			return !_blsr_u64(cmask);
		}
	};
	[[nodiscard]] bool empty() const {
		return !x;
	}
	[[nodiscard]] bool intersects(const NTSetV1& y) const {
		return (x&y.x)!=0;
	}
	int intersects(const NTSetV1& y, int* B) const {
		iterator it(x&y.x);
		if (it.atEnd())return 0;
		*B = *it;
		return it.last() ? 1 : 2;
	}
	/// Фильтрация множества нетерминалов
	bool filter(NTSetV1& y)const {
		y.x &= x;
		return !y.empty();
	}
	[[nodiscard]] bool has(unsigned i)const {
		return ((x >> i) & 1u) != 0;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return (y^x)!=0;
	}
	void add(unsigned i) {
		x |= 1ULL << i;
	}
	[[nodiscard]] int size()const {
		return (int)_mm_popcnt_u64(x);
	}
	NTSetV1& clear() { x = 0; return *this; }
	NTSetV1() :x(0) {};
	explicit NTSetV1(uint64_t v) :x(v) {};
	NTSetV1(std::initializer_list<int>& l) : x(0) { for (int i : l)add(i); }
	NTSetV1& operator|=(const NTSetV1& s) {
		x |= s.x;
		return *this;
	}
	NTSetV1& operator&=(const NTSetV1& s) {
		x &= s.x;
		return *this;
	}
	NTSetV1 operator|(const NTSetV1& s) const {
		return NTSetV1(x|s.x);
	}
	NTSetV1 operator&(const NTSetV1& s) const {
		return NTSetV1(x&s.x);
	}
	NTSetV1& operator=(const std::initializer_list<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV1& operator=(const vector<int>& l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	[[nodiscard]] iterator begin()const { return iterator(x); }
	[[nodiscard]] iterator end()const { return iterator(); }
};

template<class S1, class S2>
struct NTSetCmp {
    static constexpr int max_value = (S1::max_value < S2::max_value) ? S1::max_value : S2::max_value;
	typedef NTSetCmp<S1, S2> S;
	S1 s1; 
	S2 s2;
	void check()const {
		for (int i : s1)
			if(!s2.has(i))
				Assert(s2.has(i));
		for (int i : s2)
			if (!s1.has(i)) 
				Assert(s1.has(i));
	}
	typedef typename S2::iterator iterator;
	bool intersects(const S& y) const {
		bool b1 = s1.intersects(y.s1);
		Assert(b1 == s2.intersects(y.s2));
		return b1;
	}
	int intersects(const S& y, int* B) const {
		int B1, B2;
		int m1 = s1.intersects(y.s1, &B1);
		int m2 = s2.intersects(y.s2, &B2);
		Assert(m1 == m2);
		if (m1) {
			Assert(s2.has(B1));
			Assert(s1.has(B2));
			*B = B1;
		}
		return m1;
	}
	/// Фильтрация множества нетерминалов
    bool filter(S& y)const {
		bool b1 = s1.filter(y.s1);
		bool b2 = s2.filter(y.s2);
		y.check();
		Assert(b1 == b2);
		return b1;
	}
	[[nodiscard]] bool has(int x)const {
		bool r1 = s1.has(x);
		Assert(r1 == s2.has(x));
		return r1;
	}
	bool add_check(int x) {
		bool r1 = s1.add_check(x), r2 = s2.add_check(x);
		check();
		Assert(r1 == r2);
		return r1;
	}
	void add(int x) {
		s1.add(x); s2.add(x);
		check();
	}
	int size() {
		Assert(s1.size() == s2.size());
		return s1.size();
	}
	[[nodiscard]] bool empty()const {
		Assert(s1.empty() == s2.empty());
		return s1.empty();
	}
	S& clear() { 
		s1.clear();
		s2.clear();
		check();
		return *this; 
	}
	NTSetCmp() = default;
	NTSetCmp(const std::initializer_list<int>& l) :s1(l), s2(l) { 
		check(); 
	}
	S& operator|=(const S & s) {
		s1 |= s.s1; s2 |= s.s2;
		check();
		return *this;
	}
	S& operator&=(const S & s) {
		s1 &= s.s1; s2 &= s.s2;
		check();
		return *this;
	}
	S operator|(const S & x) const {
		S r;
		r.s1 = s1 | x.s1;
		r.s2 = s2 | x.s2;
		r.check();
		return r;
	}
	S operator&(const S & x) const {
		S r;
		r.s1 = s1 & x.s1;
		r.s2 = s2 & x.s2;
		r.check();
		return r;
	}
	S& operator=(const std::initializer_list<int> & l) {
		s1 = l;
		s2 = l;
		check();
		return *this;
	}
	S& operator=(const vector<int> & l) {
		s1 = l; s2 = l;
		check();
		return *this;
	}
	iterator begin()const { return s2.begin(); }
	iterator end()const { return s2.end(); }
};

//typedef NTSetCmp<NTSetV1,NTSetV> NTSet;
//typedef NTSetV8 NTSet;
using NTSet = NTSetV;
//typedef NTSetV2 NTSet;
//typedef NTSetV1 NTSet;
