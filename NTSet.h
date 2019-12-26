#pragma once
#include <immintrin.h>

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
	NTSetS(std::initializer_list<int>& l) :s(l) {}
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
#if 1
struct NTSetV {
	vector<u64> mask;
	struct iterator {
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
			++ptr;
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
	bool intersects(const NTSetV& y) const {
		for (int i = 0, sz = (int)std::min(y.mask.size(),mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i])return true;
		return false;
	}
	bool intersects(const NTSetV& y, int *B) const {
		for (int i = 0, sz = (int)std::min(y.mask.size(), mask.size()); i < sz; i++)
			if (mask[i] & y.mask[i]) {
				*B = (i << 6) + (int)_tzcnt_u64(mask[i] & y.mask[i]);
				return true;
			}
		return false;
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

	bool empty()const {
		for (auto x : mask)
			if (x)return false;
		return true;
	}
	NTSetV& clear() { fill(mask.begin(), mask.end(), 0); return *this; }
	NTSetV() = default;
	NTSetV(std::initializer_list<int>& l) {}
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
#endif
