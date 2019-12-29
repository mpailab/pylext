#pragma once
#include <immintrin.h>
#include "Exception.h"
struct NTSetS { // Множество нетерминалов
	unordered_set<int> s;
	typedef unordered_set<int>::const_iterator iterator;
	typedef int value_type;
	bool intersects(const NTSetS& y) const {
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
	bool has(int x)const {
		return s.count(x) > 0;
	}
	bool add(int x) {
		return s.insert(x).second;
	}
	bool add_check(int x) {
		return s.insert(x).second;
	}
	bool empty()const { return s.empty(); }
	NTSetS& clear() { s.clear(); return *this; }
	NTSetS() = default;
	NTSetS(const std::initializer_list<int>& l) :s(l) {}
	NTSetS& operator|=(const NTSetS& s) {
		for (int i : s.s)
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
	iterator begin()const { return s.begin(); }
	iterator end()const { return s.end(); }
};
typedef uint64_t u64;
struct NTSetV {
	vector<u64> mask;
	struct iterator  {
		typedef std::forward_iterator_tag iterator_category;
		typedef int value_type;
		typedef int difference_type;
		typedef int reference;
		typedef int* pointer;
		u64 curr;
		int pos = 0;
		const u64* ptr, * end;
		iterator(const u64* p, size_t l) :curr(0), ptr(p), end(p + l) {
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
		int* operator->() const {
			return 0;
		}
		bool operator==(const iterator& i)const {
			return ptr == i.ptr && curr == i.curr;
		}
		bool operator!=(const iterator& i)const {
			return ptr != i.ptr || curr != i.curr;
		}
	};
	bool intersects(const NTSetV& y) const {
		for (int i = 0, sz = (int)std::min(y.mask.size(),mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i])return true;
		return false;
	}
	int intersects(const NTSetV& y, int *B) const {
		int m = 0;
		for (int i = 0, sz = (int)std::min(y.mask.size(), mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i]) {
				*B = (i << 6) + (int)_tzcnt_u64(mask[i] & y.mask[i]);
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
		u64 r = 0;
		for (int i = 0; i < sz; i++)
			r |= y.mask[i] &= mask[i];
		return r != 0;
	}
	bool has(int x)const {
		return (x >> 6) < mask.size() && ((mask[x >> 6] >> (x & 63)) & 1) != 0;
	}
	bool add_check(int x) {
		if (has(x))return false;
		add(x);
		return true;
	}
	void add(int x) {
		int y = x >> 6;
		if (y >= mask.size())
			mask.resize(y + 1);
		mask[y] |= 1ULL << (x & 63);
	}
	int size()const {
		int r = 0;
		for (auto x : mask)
			r += (int)_mm_popcnt_u64(x);
		return r;
	}
	bool empty()const {
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
	iterator begin()const { return iterator(mask.data(), mask.size()); }
	iterator end()const { return iterator(mask.data() + mask.size(), 0); }
};

inline uint64_t extract64(__m256i x, int i) {
	switch (i) {
	case 0: return _mm256_extract_epi64(x, 0);
	case 1: return _mm256_extract_epi64(x, 1);
	case 2: return _mm256_extract_epi64(x, 2);
	case 3: return _mm256_extract_epi64(x, 3);
	}
	return 0;
}
inline __m256i bitmask256(int i) {
	//auto mask = _mm256_cmpeq_epi32(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0), _mm256_set1_epi32(i));
	return _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0), _mm256_set1_epi32(i>>5)), _mm256_set1_epi32(1U << (i & 31)));
}

struct NTSetV4 {
	//union {
		//u64 mask[4];
		__m256i x;
	//};
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
			pmask = _mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(x, _mm256_setzero_si256())))^0xF;
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
			return (int)_tzcnt_u64(cmask) + _tzcnt_u32(pmask)*64;
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
		return _mm256_testz_si256(x, x)!=0;
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
	/// Фильтрация множества нетерминалов
	bool filter(NTSetV4& y)const {
		y.x = _mm256_and_si256(x, y.x);
		return !y.empty();
	}
	int has(int i)const {
		return int(extract64(x, i >> 6) >> (i&63)) & 1;
	}
	bool add_check(int i) {
		auto y = x;
		add(i);
		return !_mm256_testc_si256(y,x);
	}
	void add(int i) {
		x = _mm256_or_si256(x, bitmask256(i));
	}
	int size()const {
		return int(_mm_popcnt_u64(_mm256_extract_epi64(x, 0)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 1)) + 
			_mm_popcnt_u64(_mm256_extract_epi64(x, 2)) + _mm_popcnt_u64(_mm256_extract_epi64(x, 3)));
	}
	NTSetV4& clear() { x=_mm256_setzero_si256(); return *this; }
	NTSetV4() :x(_mm256_setzero_si256()) {};
	NTSetV4(__m256i v) :x(v) {};
	NTSetV4(std::initializer_list<int>& l) { for (int i : l)add(i); }
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


template<class S1, class S2>
struct NTSetCmp {
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
	bool has(int x)const {
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
	bool empty()const {
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