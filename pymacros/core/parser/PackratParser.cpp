#include <algorithm>
#include "PackratParser.h"
#include "Exception.h"
#include "format.h"

void PEGGrammar::update_props() {
    for (auto& r : rules)
        r->_updatecmplx(&rules, true);
	_updated = true;
	_ops = 0;
}

void PEGGrammar::add_rule(const string & nt, const PEGExpr & e, bool to_begin) {
	int a = _en[nt];
	while (a >= len(rules)){
	    rules.resize(a + 1);
	}
	if(!rules[a])rules[a] = make_unique<PEGExpr>();
	if (to_begin)*rules[a] = e / *rules[a];
	else *rules[a] /= e;
	_updateHash(*rules[a]);
	_updated = false;
	_ops = 0;
}

void PackratParser::setText(const string & t) {
	text = t;
	lastpos = (int)t.size();
	acceptedh.clear();
	manyh.clear();
}

static constexpr int iterMemStart = 8; // Начинаем запоминать после 8-го прочитанного элемента 
static constexpr unsigned int cmplxThresh = 16; // Запоминаем выражение со сложностью выше этого значения 

int PackratParser::parse0(const PEGExpr & e, int pos) {
	if (!peg->_updated)peg->_ops++;
	switch (e.type) {
	case PEGExpr::OrdAlt:
		for (auto &e1 : e.subexprs)
			if (int a = parse(e1, pos))
				return a;
		return 0;
	case PEGExpr::Opt:
		if (int a = parse(e.subexprs[0], pos))return a;
		else return pos;
	case PEGExpr::Concat:
		for (auto &e1 : e.subexprs) {
			if (int a = parse(e1, pos))
				pos = a;
			else return 0;
		}
		return pos;
	case PEGExpr::String:
		for (int i = 0; e.s[i]; i++)
			if (pos+i>lastpos || text[pos + i - 1] != e.s[i])return 0;
		return pos + len(e.s);
	case PEGExpr::Many1:
	case PEGExpr::Many:
		if (e.subexprs[0].type == PEGExpr::Terminal) {
			constexpr int termMemFreq = 4 * iterMemStart;
			int i;
			int p0 = pos;
			auto& m = e.subexprs[0].t_mask;
			for (i = 0; i < termMemFreq && pos <= lastpos; i++, pos++) {
				if (!m[(unsigned char)text[pos-1]])break;
			}
			if (i == termMemFreq) {
				int id = e.subexprs[0].id;
				auto mp = _manypos.size();
				for (; pos<=lastpos; pos++) {
					if (!m[(unsigned char)text[pos - 1]]) break;
					if (!(pos & (termMemFreq - 1u))) {
						//int& aa = hmany(pos, id);
						if (int aa = hmany(pos,id)) {
							pos = aa; break;
						}
						_manypos.push_back(pos);// &aa);
					}
				}
				for (auto j = mp; j < _manypos.size(); j++)hmany(_manypos[j],id) = pos;
				hmany((p0 + termMemFreq - 1u) & ~(termMemFreq - 1u), id) = pos;
				_manypos.resize(mp);
			}
			return (pos > p0 || e.type == PEGExpr::Many || i>0) ? pos : 0;
		} else {
			int one = 0, i;
			int p0 = pos;
			auto& e1 = e.subexprs[0];
			for (i = 0; i < iterMemStart; i++) {
				if (int a = parse(e1, pos)) {
					one = 1;
					if (a == pos) break;
					pos = a;
				} else break;
			}
			if (i == iterMemStart) {
				auto mp = _manypos.size();
				for (;;i++) {
					if (i % iterMemStart == 0) {
						if (int aa = hmany(pos, e1.id)) { pos = aa; break; }
					}
					if (int a = parse(e1, pos)) {
						if(i/iterMemStart%iterMemStart==0)
							_manypos.push_back(pos);// &aa);
						if (a == pos) break;
						pos = a;
					} else break;
				}
				for (auto j = mp; j < _manypos.size(); j++)hmany(_manypos[j], e1.id) = pos;
				hmany(p0, e1.id) = pos;
				_manypos.resize(mp);
			}
			return (pos > p0 || e.type == PEGExpr::Many || one) ? pos : 0;
		}
	case PEGExpr::PosLookahead:
		return parse(e.subexprs[0], pos) ? pos : 0;
	case PEGExpr::NegLookahead:
		return parse(e.subexprs[0], pos) ? 0 : pos;
	case PEGExpr::Terminal:
		return (pos<=lastpos && e.t_mask[(unsigned char)text[pos-1]]) ? pos + 1 : 0;
	case PEGExpr::NonTerminal:
		if (int a = parse(e.num, pos)) {		
			return a < 0 ? 0 : a;
		}
	case PEGExpr::Empty:
		return 0;
	}
	return 0;
}

int PackratParser::parse(const PEGExpr & e, int pos) {
	int r = parse0(e, pos);
	if (!r)err_at(&e, pos-1);
	return r;
}

int PackratParser::parse(int nt, int pos) {
	if (!peg->_updated) {
		if (peg->_ops > peg->rules.size() * 100)
			peg->update_props();
	} else if (!peg->rules[nt]->t_mask[(unsigned char)text[pos - 1]])
		return -1;
	if (peg->rules[nt]->_cmplx <= (int)cmplxThresh) {
		int r = parse(*peg->rules[nt], pos);
		return r ? r : -1;
	}
	int &a = accepted(pos, nt);
	if (a == -2)throw Exception("Left recursion not allowed in PEG, detected at position {} in nonterminal {}"_fmt(pos, peg->_en[nt]));
	if (a)return a;
	a = -2; // Помечаем, что начали разбирать нетерминал nt на позиции pos, чтобы можно было обнаружить зацикливание по рекурсии.
	int r = parse(*peg->rules[nt], pos);
	return accepted(pos,nt) = (r ? r : -1);
}

bool PackratParser::parse(int nt, int pos, int &end, string * res) {
	int a = parse(nt, pos+1);
	if (a > pos) {
		if(res)*res = text.substr(pos, a - 1 - pos);
		end = a - 1;
	}
	return a > pos;
}

inline const char*& ws(const char*& s) {
	while (isspace(*s))s++;
	return s;
}

PEGExpr readstr(const char *&s, const char *&errpos, string *err) {
	char c = *s;
	string res;
	for (s++; *s&&*s != c; s++) {
		if (*s == '\\') {
			switch (*++s) {
			case 'n':res += '\n'; break;
			case 'b':res += '\b'; break;
			case 't':res += '\t'; break;
			case 'r':res += '\r'; break;
			case 'v':res += '\v'; break;
			case 'a':res += '\a'; break;
			default: res += *s;
			}
		} else if (*s == '\n') {
			errpos = s;
			if (err)*err = c + " expected before newline"s;
			return{};
		} else res += *s;
	}
	if (*s != c) {
		errpos = s;
		if (err)*err = c + " expected"s;
		return{};
	}
	s++;
	return PEGExpr(res);
}

PEGExpr readsym(const char *&s, const char *&errpos, string *err) {
	const char *b = s;
	if (*s != '[') {
		errpos = s;
		if (err)*err = "'[' expected";
		return{};
	}
	++s;
	bitset<256> res(0ULL);
	bool val = true;
	if (*s == '^') {
		res.flip();
	}
	unsigned char prev=0, curr;
	bool rng = false,was_prev = false;
	for (; *s&&*s != ']'; s++) {
		if (*s == '^') { val = false; continue; }
		if (*s == '-') {
			if (!was_prev) {
				if (err)*err = "start of range expected";
				return {};
			}
			rng = true; continue;
		}
		if (*s == '\\') {
			switch (*++s) {
			case 'n':res[curr = uint8_t('\n')] = val; break;
			case 'b':res[curr = uint8_t('\b')] = val; break;
			case 't':res[curr = uint8_t('\t')] = val; break;
			case 'r':res[curr = uint8_t('\r')] = val; break;
			case 'v':res[curr = uint8_t('\v')] = val; break;
			case 'a':res[curr = uint8_t('\a')] = val; break;
			default: res[curr = uint8_t(*s)] = val; break;
			}
		} else if (*s == '\n') {
			errpos = s;
			if (err)*err = "']' expected before newline"s;
			return{};
		} else {
			res[curr = uint8_t(*s)] = val;
		}
		if (rng) {
			if (prev > curr) {
				errpos = s;
				if (err)*err = "empty range ["s + (char)prev + "-" + (char)curr + "]";
			}
			for (; prev <= curr; prev++)
				res[prev] = val;
			rng = false;
		}
		prev = curr;
		was_prev = true;
	}
	if (rng) {
		errpos = s;
		if (err)*err = "range end expected";
	}
	if (*s != ']') {
		errpos = s;
		if (err)*err = "']' expected"s;
		return{};
	}
	s++;
	return PEGExpr(res,string(b,s-b));
}

PEGExpr readexpr(PEGGrammar*p, const char *&s, const char *&errpos, string *err, char end = 0);
PEGExpr readatomexpr(PEGGrammar *p, const char *&s, const char *&errpos, string *err) {
	PEGExpr res;
	switch (*ws(s)) {
	case '"':
	case '\'':
		return readstr(s,errpos,err);
	case '[':
		return readsym(s,errpos,err);
	case '!':
		return !readatomexpr(p,++s, errpos, err);
	case '&':
		return lookahead(readatomexpr(p,++s, errpos, err));
	case '(':
		return readexpr(p,++s, errpos, err, ')');	
	}
	if (!isalpha(*s)&&*s!='_') {
		errpos = s;
		if (err)*err = "unexpected symbol `"s + *s + "`";
		return{};
	}
	const char *b = s;
	while (isalnum(*s)||*s=='_')s++;
	int id = p->_en[string(b, s - b)];
	return pnonterm(id, p->_en[id]);
}

PEGExpr readpostfixexpr(PEGGrammar*p, const char *&s, const char *&errpos, string *err) {
	PEGExpr t = readatomexpr(p,s, errpos, err);
	if (errpos)return t;
	while (*ws(s)) {
		switch (*ws(s)) {
		case '*': t = many(move(t)); ++s; continue;
		case '?': t = maybe(move(t)); ++s; continue;
		case '+': t = many1(move(t)); ++s; continue;
		}
		break;
	}
	return t;
}

PEGExpr readconcatexpr(PEGGrammar*p, const char *&s, const char *&errpos, string *err, char end) {
	PEGExpr res = readpostfixexpr(p, s, errpos, err);
	if (errpos)return res;
	while (*ws(s) && *s!=end && *s != '/') {
		const char *ps = s;
		PEGExpr t = readpostfixexpr(p, s, errpos, err);
		if (errpos) {
			if (s == ps)errpos = 0;
			else return{};
			break;
		}
		res *= t;
	}
	return res;
}

PEGExpr readexpr(PEGGrammar*p, const char *&s, const char *&errpos, string *err, char end) {
	PEGExpr res = readconcatexpr(p, s, errpos, err, end);
	if (errpos)return res;
	while(*ws(s)=='/') {
		res /= readconcatexpr(p, ws(++s), errpos, err, end);
		if (errpos)return res;
	}
	if (*s != end) {
		errpos = s;
		if(err)*err = (end ? "'"s + end + "'" : "end of parsing expression"s) + " expected";
		return{};
	}
	if(end)++s;
	return res;
}

PEGExpr readParsingExpr(PEGGrammar*p, const string & s, int *errpos, string * err) {
	const char * ep=0, *ps = s.c_str();
	PEGExpr r = *ws(ps)=='`' ? readexpr(p, ++ps, ep, err,'`') : readexpr(p, ps, ep, err);
	if (ep) {
		if (errpos)*errpos = int(ep - s.c_str());
		return{};
	}
	if (*ws(ps)) {
		if (errpos)*errpos = int(ps - s.c_str());
		if (err)*err = "unexpected symbol at the end of parsing expression";
		return{};
	}
	if (errpos)*errpos = -1;
	return r;
}
