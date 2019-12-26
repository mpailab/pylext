#pragma once
#include <vector>
#include <memory>
#include <bitset>
#include "common.h"
using namespace std;

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
	PEGExpr() = default;
	PEGExpr(const string &ss):type(String),s(ss) {}
	PEGExpr(const bitset<256> & bs, const string&text = "") :type(Terminal), t_mask(bs), s(text) {}
	PEGExpr(Type t, vector<PEGExpr> && l, const string&text = ""):type(t),subexprs(move(l)),s(text) {}
	PEGExpr& operator /=(PEGExpr &&e) {
		if (type == Empty)return *this = move(e);
		if (type == OrdAlt)subexprs.emplace_back(move(e));
		else {
			subexprs = { std::move(*this), std::move(e) };
			type = OrdAlt;
		}
		return *this;
	}
	PEGExpr& operator *=(PEGExpr &&e) {
		if (type == Empty)return *this = move(e);
		if (type == Concat) {
			if (subexprs.back().type == Terminal&&e.type == Terminal)
				subexprs.back().t_mask |= e.t_mask;
			else subexprs.emplace_back(move(e));
		} else if (type == Terminal&&e.type == Terminal) {
			t_mask |= e.t_mask;
		} else if (type == String&&e.type == String) {
			s += e.s;
		} else {
			subexprs = { std::move(*this), std::move(e) };
			type = Concat;
		}
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
				if (!(subexprs[i] == e.subexprs[1]))
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

struct PackratParser {
	int errpos;
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
	vector<int*> _manypos;
	struct HashExpr {
		mutable PackratParser *p = 0;
		HashExpr(PackratParser *ps) :p(ps) {}
		size_t operator()(const PEGExpr *e)const {
			size_t h = std::hash<int>()(e->type);
			switch (e->type) {
			case PEGExpr::Terminal:
				return h ^ e->t_mask.hash();
			case PEGExpr::String:
				return h^std::hash<string>()(e->s);
			case PEGExpr::NonTerminal:
				return h ^ std::hash<int>()(e->num);
			default:
				for (auto &x : e->subexprs) {
					if (x.id < 0)x.id = p->_een[&x];
					h ^= std::hash<int>()(x.id);
				}
			}
			return h;
		}
	};
	struct EqExpr {
		bool operator()(const PEGExpr *e1, const PEGExpr *e2)const { return *e1 == *e2; }
	};
	Enumerator<string,unordered_map> _en;
	Enumerator<const PEGExpr*, unordered_map, HashExpr,EqExpr> _een;
	vector<PEGExpr> rules;
	//vector<vector<int>> accepted;
	unordered_map<uint64_t, int> acceptedh;
	unordered_map<uint64_t, int> manyh;
	int &hmany(uint32_t pos, uint32_t id) {
		return manyh[(uint64_t(pos) << 32) | id];
	}
	int &accepted(uint32_t pos, uint32_t id) {
		return acceptedh[(uint64_t(pos) << 32) | id];
	}
	string text;
	//int pos;
	void add_rule(const string &nt, const PEGExpr &e);
	PackratParser() :_een(1024,HashExpr(this)) {}
	void setText(const string &t);
	int parse(const PEGExpr&e, int pos);
	int parse0(const PEGExpr&e, int pos);
	int parse(int nt, int pos);
	bool parse(int nt, int pos, int &end, string *res);
};

PEGExpr readParsingExpr(PackratParser*p, const string & s, int *errpos, string * err);
