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
	string s;
	//size_t min = 0, max = -1;
	int num = 0;
	mutable int id=-1;
	PEGExpr() = default;
	PEGExpr(const string &ss):type(String),s(ss) {}
	PEGExpr(const bitset<256> & bs) :type(Terminal), t_mask(bs) {}
	PEGExpr(Type t, vector<PEGExpr> && l):type(t),subexprs(move(l)) {}
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
};
PEGExpr operator!(PEGExpr &&e) {
	return PEGExpr(PEGExpr::NegLookahead, { move(e) });
}
PEGExpr lookahead(PEGExpr &&e) {
	return PEGExpr(PEGExpr::PosLookahead, { move(e) });
}
PEGExpr many(PEGExpr && e) {
	return PEGExpr(PEGExpr::Many, { move(e) });
}

PEGExpr many1(PEGExpr && e) {
	return PEGExpr(PEGExpr::Many1, { move(e) });
}

PEGExpr maybe(PEGExpr && e) {
	return PEGExpr(PEGExpr::Opt, { move(e) });
}

PEGExpr pstr(const string &s) {
	return PEGExpr(s);
}

PEGExpr pnonterm(int n) {
	PEGExpr res;
	res.type = PEGExpr::NonTerminal;
	res.num = n;
	return res;
}

PEGExpr readexpr(PackratParser*p, const string & s, int *errpos, string * err);

struct PackratParser {
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
	int parse(int nt, int pos);
	bool parse(int nt, int pos, string *res);
};
