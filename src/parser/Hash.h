#pragma once
#include <vector>
using namespace std;

struct hash64 {
	inline size_t operator()(uint64_t v) const noexcept {
		size_t x = 14695981039346656037ULL;
		for (size_t i = 0; i < 64; i += 8) {
			x ^= static_cast<size_t>((v >> i) & 255u);
			x *= 1099511628211ULL;
		}
		return x;
	}
	inline size_t operator()(uint32_t v) const noexcept {
		size_t x = 14695981039346656037ULL;
		for (size_t i = 0; i < 32; i += 8) {
			x ^= static_cast<size_t>((v >> i) & 255u);
			x *= 1099511628211ULL;
		}
		return x;
	}
	inline size_t operator()(int v) const noexcept {
		return (*this)(uint32_t(v));
	}
};

template<class K, class V, class H = hash64>//std::hash<K>>
struct PosHash {
	struct Elem {
		int next = 0;
		/*union {
			int h;
			int prev = 0;
		};*/
		V val;
		K key;
	};
	H h;
	vector<Elem> _elems;
	V _def = V();
	unsigned _size = 0, _nbin = 0, _mask = 0, _free = 0;
	int _coef = 2;
	//unordered_map<K, V, H> _check;
	PosHash(PosHash &&x) noexcept :_elems(move(x._elems)),h (x.h), _def(x._def), _size(x._size),_nbin(x._nbin),_mask(x._mask),_free(x._free),_coef(x._coef) {
		x._nbin = 0;
		x._mask = 0;
		x._free = 0;
		x._size = 0;
	}
	PosHash() = default;
	void clear() {
		//_check.clear();
		for (int i = 0; i < _nbin; i++)
			_elems[i].next = -1;
		for (int i = _nbin+1; i < 2 * _nbin; i++)
			_elems[i-1].next = i;
		_mask = _nbin - 1;
		_free = _nbin;
		_size = 0;
	}
	void rehash() {
		unsigned sz = 1;
		while (sz < (_size + 1) * _coef)sz <<= 1u;
		if (sz == _nbin)return;
		vector<Elem> old = move(_elems);
		_elems.clear();
		_nbin = sz;
		_elems.resize(2 * sz);
		for (int i = 0; i < sz; i++)
			_elems[i].next = -1;
		for (unsigned i = sz; i < 2 * sz - 1; i++)
			_elems[i].next = i + 1;
		_elems[2 * sz - 1].next = -1;
		_mask = _nbin - 1;
		int oldfree = _free;
		_free = sz;
		_size = 0;
		for (int i = 0; i < oldfree; i++) {
			if (old[i].next >= 0)
				_insert(old[i].key, old[i].val);
		}
	}
	Elem& _insert(K k, V v) {
		/*struct Counter {
			int cnt = 0;
			~Counter() {
				cout << "Count = " << cnt << endl;
			}
			void operator++() { cnt++; }
		};*/
		//static Counter c;
		//++c;
		if (_size*_coef >= _nbin)rehash();
		int hh = h(k)&_mask;
		if (_elems[hh].next < 0) {
			_size++;
			_elems[hh].key = k;
			_elems[hh].val = v;
			_elems[hh].next = 0;
			return _elems[hh];
		}
		for (;; hh = _elems[hh].next) {
			if (_elems[hh].key == k)return _elems[hh];
			if (!_elems[hh].next) break;
		}
		_size++;
		hh = _elems[hh].next = _free;
		_free = _elems[hh].next;
		_elems[hh].key = k;
		_elems[hh].val = v;
		_elems[hh].next = 0;
		return _elems[hh];
	}
	bool insert(K k, V v) {
		Elem& e = _insert(k, v);
		//Assert((e.next < 0) == _check.insert(make_pair(k, v)).second);
		if (e.next < 0) {
			//_check[k] = v; //Check
			e.next = 0;
			e.val = v;
			return true;
		} else return false;
	}
	V& operator[](K k) {
		Elem& e = _insert(k, _def);
		//_check[k]=_def;
		//Assert(_check.size() == _size);
		//Assert(_check.insert(make_pair(k, _def)).first->second == e.val);
		return e.val;
	}
	[[nodiscard]] const V& find(K k)const {
		int hh = h(k) & _mask;
		if (!_elems.size() || _elems[hh].next < 0) {
			//Assert(!_check.count(k));
			return _def;
		}
		for (;; hh = _elems[hh].next) {
			if (_elems[hh].key == k) {
				//Assert(_check.count(k));//&& _check.find(k)->second == _elems[hh].val);
				return _elems[hh].val;
			}
			if (!_elems[hh].next) break;
		}
		//Assert(!_check.count(k));
		return _def;
	}
	[[nodiscard]] const V* findp(K k)const {
		int hh = h(k) & _mask;
		if (!_elems.size() || _elems[hh].next < 0) return 0;
		for (;; hh = _elems[hh].next) {
			if (_elems[hh].key == k)return &_elems[hh].val;
			if (!_elems[hh].next) break;
		}
		return 0;
	}
	[[nodiscard]] int size()const { return _size; }
};