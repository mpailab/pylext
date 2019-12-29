#pragma once
#include <vector>
#include <memory>
#include <bitset>
#include <unordered_map>
#include <iostream>
#include "common.h"
#include "Exception.h"
using namespace std;

struct hash64 {
	inline size_t operator()(uint64_t v) noexcept {
		size_t x = 14695981039346656037ULL;
		for (size_t i = 0; i < 64; i += 8) {
			x ^= static_cast<size_t>((v >> i) & 255);
			x *= 1099511628211ULL;
		}
		return x;
	}
};

template<class K, class V, class H=hash64>//std::hash<K>>
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
	int _size=0, _nbin = 0, _mask = 0, _free = 0;
	int _coef = 2;
	//unordered_map<K, V, H> _check;
	void clear() {
		//_check.clear();
		for (int i = 0; i < _nbin; i++)
			_elems[i].next = -1;
		for (int i = _nbin; i < 2 * _nbin - 1; i++)
			_elems[i].next = i + 1;
		_mask = _nbin - 1;
		_free = _nbin;
		_size = 0;
	}
	void rehash() {
		int sz = 1;
		while (sz < (_size+1) * _coef)sz <<= 1;
		if (sz == _nbin)return;
		vector<Elem> old = move(_elems);
		_elems.clear();
		_nbin = sz;
		_elems.resize(2 * sz);
		for (int i = 0; i < sz; i++)
			_elems[i].next = -1;
		for (int i = sz; i < 2*sz-1; i++)
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
		struct Counter {
			int cnt = 0;
			~Counter() {
				cout << "Count = " << cnt << endl;
			}
			void operator++() { cnt++; }
		};
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
		for(;;hh=_elems[hh].next) {
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
		Elem& e = _insert(k,v);
		//Assert((e.next < 0) == _check.insert(make_pair(k, v)).second);
		if (e.next < 0) {
			//_check[k] = v; //Check
			e.next = 0;
			e.val = v;
			return true;
		} else return false;
	}
	V& operator[](K k) {
		Elem& e = _insert(k,_def);
		//_check[k]=_def;
		//Assert(_check.size() == _size);
		//Assert(_check.insert(make_pair(k, _def)).first->second == e.val);
		return e.val;
	}
	const V& find(K k)const{
		int hh = h(k) & _mask;
		if (_elems[hh].next < 0) {
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
	const V* findp(K k)const {
		int hh = h(k) & _mask;
		if (_elems[hh].next < 0) return 0;
		for (;; hh = _elems[hh].next) {
			if (_elems[hh].key == k)return &_elems[hh].val;
			if (!_elems[hh].next) break;
		}
		return 0;
	}
};

struct PEGExpr {
	enum Type {
		Empty = 0,
		OrdAlt,
		PosLookahead,
		NegLookahead,
		Opt,
		Concat,
		Many,
		Many1,
		Terminal,
		String,
		NonTerminal
	};
	Type type = Empty;
	vector<PEGExpr> subexprs;
	bitset<256> t_mask;
	mutable string s;
	//size_t min = 0, max = -1;
	int num = 0;
	mutable int id=-1;
	int _cmplx = -1;
	void _updatecmplx(const vector<PEGExpr> *v = 0, bool rec = false) {
		if (type != Terminal) {
			t_mask.reset();
			t_mask.flip();
		}
		if (rec) {
			for (auto& e : subexprs)
				e._updatecmplx(v, true);
		}
		switch (type) {
		case Terminal:
			_cmplx = 1;
			return;
		case String:
			_cmplx = (int)s.size();
			t_mask.reset();
			t_mask[(unsigned char)s[0]] = true;
			return;
		case NonTerminal:
			if (v) {
				if (num < (int)v->size()) {
					_cmplx = (*v)[num]._cmplx;
					t_mask = (*v)[num].t_mask;
				} else t_mask.reset(), _cmplx = 1;
			}
			else _cmplx = -1;
			return;
		case Many1:
			t_mask = subexprs[0].t_mask;
		case Many:
			_cmplx = -1;
			return;
		case OrdAlt:
			t_mask = subexprs[0].t_mask;
			for (auto& e : subexprs)
				t_mask |= e.t_mask;
			break;
		case Concat:
			if ((subexprs[0].type == Opt || subexprs[0].type == Many) && subexprs.size() > 1) {
				t_mask = subexprs[0].subexprs[0].t_mask | subexprs[1].t_mask;
			} else if (subexprs[0].type == NegLookahead) t_mask = subexprs[1].t_mask;
			else if (subexprs[0].type == PosLookahead)t_mask = subexprs[0].t_mask & subexprs[1].t_mask;
			else t_mask = subexprs[0].t_mask;
			break;
		case PosLookahead:
			t_mask = subexprs[0].t_mask;
			break;
		default:;
		}
		_cmplx = 1;
		for (auto& c : subexprs)
			_cmplx = ((_cmplx < 0 || c._cmplx < 0) ? -1 : _cmplx + c._cmplx);
	}
	PEGExpr() = default;
	PEGExpr(const string &ss):type(String),s(ss),_cmplx((int)ss.size()) {}
	PEGExpr(const bitset<256> & bs, const string&text = "") :type(Terminal), t_mask(bs), s(text),_cmplx(1) {}
	PEGExpr(Type t, vector<PEGExpr> && l, const string&text = ""):type(t),subexprs(move(l)),s(text) {
		_updatecmplx();
	}
	PEGExpr& operator /=(PEGExpr &&e) {
		if (type == Empty)return *this = move(e);
		if (type == OrdAlt) {
			subexprs.emplace_back(move(e));
		} else {
			subexprs = { std::move(*this), std::move(e) };
			type = OrdAlt;
		}
		id = -1;
		_updatecmplx();
		return *this;
	}
	PEGExpr& operator *=(PEGExpr &&e) {
		if (type == Empty)return *this = move(e);
		if (type == Concat) {
			if (e.type == Concat) {
				if (e.subexprs[0].type == String && subexprs.back().type == String) {
					subexprs.back().s += e.subexprs[0].s;
					subexprs.back().id = -1;
					subexprs.insert(subexprs.end(), e.subexprs.begin() + 1, e.subexprs.end());
				} else subexprs.insert(subexprs.end(), e.subexprs.begin(), e.subexprs.end());
			} else if (subexprs.back().type == String && e.type == String) {
				subexprs.back().s += e.s;
				subexprs.back().id = -1;
			} else subexprs.emplace_back(move(e));
		} else if (type == Terminal&&e.type == Terminal) {
			t_mask |= e.t_mask;
		} else if (type == String&&e.type == String) {
			s += e.s;
		} else {
			subexprs = { std::move(*this), std::move(e) };
			type = Concat;
		}
		id = -1;
		_updatecmplx();
		return *this;
	}
	PEGExpr& operator /=(const PEGExpr &e) { return (*this) /= PEGExpr(e); }
	PEGExpr& operator *=(const PEGExpr &e) { return (*this) *= PEGExpr(e); }
	PEGExpr operator*(const PEGExpr &e)const {
		PEGExpr res = *this;
		return res *= e;
	}
	PEGExpr operator/(const PEGExpr &e)const {
		PEGExpr res = *this;
		return res /= e;
	}
	PEGExpr operator!()const {
		return PEGExpr(NegLookahead, { *this });
	}
	bool operator==(const PEGExpr &e)const {
		if (id >= 0 && e.id >= 0)return id == e.id;
		if (type != e.type)return false;
		if (this == &e)return true;
		switch (type) {
		case PEGExpr::Terminal:
			return t_mask == e.t_mask;
		case PEGExpr::String:
			return s == e.s;
		case PEGExpr::NonTerminal:
			return num == e.num;
		default:
			if (subexprs.size() != e.subexprs.size())return false;
			for (int i = 0; i < (int)subexprs.size(); i++)
				if (!(subexprs[i] == e.subexprs[i]))
					return false;
		}
		return true;
	}
	string str()const {
		if (!s.empty())return s;
		switch (type) {
		case Empty:
			return s = "<empty>";
		case Opt:
			return s=subexprs[0].str() + '?';
		case Many:
			return s = subexprs[0].str() + '*';
		case Many1:
			return s = subexprs[0].str() + '+';
		case PosLookahead:
			return s = '&'+subexprs[0].str();
		case NegLookahead:
			return s = '!'+subexprs[0].str();
		case OrdAlt:
			s = '('+subexprs[0].str();
			for (int i = 1; i < len(subexprs); i++)
				s += " / " + subexprs[i].str();
			return s += ')';
		case Concat:
			s = '(' + subexprs[0].str();
			for (int i = 1; i < len(subexprs); i++)
				s += ' ' + subexprs[i].str();
			return s += ')';
		}
		return s;
	}
};
inline PEGExpr operator!(PEGExpr &&e) {
	return PEGExpr(PEGExpr::NegLookahead, { move(e) });
}
inline PEGExpr lookahead(PEGExpr &&e) {
	return PEGExpr(PEGExpr::PosLookahead, { move(e) });
}
inline PEGExpr many(PEGExpr && e) {
	return PEGExpr(PEGExpr::Many, { move(e) });
}

inline PEGExpr many1(PEGExpr && e) {
	return PEGExpr(PEGExpr::Many1, { move(e) });
}

inline PEGExpr maybe(PEGExpr && e) {
	return PEGExpr(PEGExpr::Opt, { move(e) });
}

inline PEGExpr pstr(const string &s) {
	return PEGExpr(s);
}

inline PEGExpr pnonterm(int n, const string&text = "") {
	PEGExpr res;
	res.s = text;
	res.type = PEGExpr::NonTerminal;
	res.num = n;
	return res;
}
template<class T>
inline size_t get_hash(const T& x) {
	return hash<T>()(x);
}

struct PackratParser {
	int errpos;
	int lastpos=0;
	vector<const PEGExpr*> errin;
	void reseterr() { errpos = 0; errin.clear(); }
	void err_at(const PEGExpr*e, int pos) {
		if (errpos < pos) {
			errpos = pos;
			errin.clear();
		}
		if (errpos == pos)errin.push_back(e);
	}
	string err_variants()const {
		string res;
		for (auto *e : errin)
			res += (res.empty() ? "" : ", ") + e->str();
		return res;
	}
	bool _updated=true;
	int _ops = 0;
	vector<int> _manypos;
	struct HashExpr {
		mutable PackratParser *p = 0;
		HashExpr(PackratParser *ps) :p(ps) {}
		size_t operator()(const PEGExpr *e)const {
			size_t h = std::hash<int>()(e->type);
			switch (e->type) {
			case PEGExpr::Terminal:
				return h ^ get_hash(e->t_mask);
			case PEGExpr::String:
				return h^std::hash<string>()(e->s);
			case PEGExpr::NonTerminal:
				return h ^ std::hash<int>()(e->num);
			default:
				for (auto &x : e->subexprs) {
					//Assert(x.id>=0);
					//if (x.id < 0)x.id = p->_een[&x];
					h ^= std::hash<int>()(x.id);
				}
			}
			return h;
		}
	};
	struct EqExpr {
		bool operator()(const PEGExpr *e1, const PEGExpr *e2)const { return *e1 == *e2; }
	};
	void _updateHash(PEGExpr& e) {
		for (auto& x : e.subexprs)
			if (x.id < 0)_updateHash(x);
		e.id = _een[&e];
	}
	Enumerator<string,unordered_map> _en;
	Enumerator<const PEGExpr*, unordered_map, HashExpr,EqExpr> _een;
	vector<PEGExpr> rules;
	//vector<vector<int>> accepted;
	PosHash<uint64_t, int> acceptedh;
	PosHash<uint64_t, int> manyh;
	//unordered_map<uint64_t, int> acceptedh;
	//unordered_map<uint64_t, int> manyh;
	int &hmany(uint32_t pos, uint32_t id) {
		return manyh[(uint64_t(pos) << 32) | id];
	}
	int &accepted(uint32_t pos, uint32_t id) {
		return acceptedh[(uint64_t(pos) << 32) | id];
	}
	string text;
	//int pos;
	void update_props();
	void add_rule(const string &nt, const PEGExpr &e, bool to_begin = false);
	PackratParser() :_een(1024,HashExpr(this)) {}
	void setText(const string &t);
	int parse(const PEGExpr&e, int pos);
	int parse0(const PEGExpr&e, int pos);
	int parse(int nt, int pos);
	bool parse(int nt, int pos, int &end, string *res);
};

PEGExpr readParsingExpr(PackratParser*p, const string & s, int *errpos, string * err);
