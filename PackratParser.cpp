#include "PackratParser.h"
#include "Exception.h"

void PackratParser::add_rule(const string & nt, const PEGExpr & e) {
	int a = _en[nt];
	if (a >= len(rules))rules.resize(a + 1);
	e.id = _een[&e];
	rules[a] = e;
}

void PackratParser::setText(const string & t) {
	text = t;
	//accepted = vector<vector<int>>(t.size());
}

int PackratParser::parse(const PEGExpr & e, int pos) {
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
	case PEGExpr::Many1:
	case PEGExpr::Many:{
		_manypos.clear();
		size_t i;
		int p0 = pos;
		auto &e1 = e.subexprs[0];
		for (i = 0; ; i++) {
			int &aa = hmany(pos, e1.id);
			if (aa) { pos = aa; i++;  break; }
			if (int a = parse(e1, pos)) {
				_manypos.push_back(&aa);
				if (a == pos) {
					i++; break;
				} else pos = a;
			} else break;
		}
		for (int *x : _manypos)*x = pos;
		return (pos>p0 || e.type==PEGExpr::Many || i>0) ? pos : 0;
	}
	case PEGExpr::PosLookahead:
		return parse(e.subexprs[0], pos) ? pos : 0;
	case PEGExpr::NegLookahead:
		return parse(e.subexprs[0], pos) ? 0 : pos;
	case PEGExpr::Terminal:
		return e.t_mask[text[pos-1]] ? pos + 1 : 0;
	case PEGExpr::NonTerminal:
		if (int a = parse(e.num, pos)) {		
			return a < 0 ? 0 : a;
		}
	}
	return 0;
}

int PackratParser::parse(int nt, int pos) {
	int &a = accepted(pos, nt);
	if (a == -2)throw Exception("Left recursion not allowed in PEG, detected at position "+to_string(pos)+" in nonterminal "+_en[nt]);
	if (a)return a;
	a = -2; // Помечаем, что начали разбирать нетерминал nt на позиции pos, чтобы можно было обнаружить зацикливание по рекурсии.
	//if (nt >= (int)accepted.size())
	//	accepted.resize(nt + 100);
	//else if(pos >= (int)accepted[nt].size())
	//	accepted[nt].resize(nt + 1,-1);
	//if (int a = accepted[pos][nt]) return a;
	int r = parse(rules[nt], pos);
	return a = (r ? r : -1);
}

bool PackratParser::parse(int nt, int pos, string * res) {
	int a = parse(nt, pos+1);
	if (res && a > pos)*res = text.substr(pos, a - 1 - pos);
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
			}
		} else if (*s == '\n') {
			errpos = s;
			if (err)*err = c + " expected before newline"s;
			return{};
		} else res += *s;
	}
	if (!*s) {
		errpos = s;
		if (err)*err = c + " expected"s;
		return{};
	}
	s++;
	return PEGExpr(res);
}
PEGExpr readsym(const char *&s, const char *&errpos, string *err) {
	if (*s != '[') {
		errpos = s;
		if (err)*err = "'[' expected";
		return{};
	}
	bitset<256> res;
	bool val = true;
	if (*s == '^')res.flip();
	unsigned char prev, curr;
	bool rng = false;
	for (s++; *s&&*s != ']'; s++) {
		if (*s == '^') { val = false; }
		if (*s == '-') { rng = true; continue; }
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
	return PEGExpr(res);
}

PEGExpr readexpr(PackratParser*p, const char *&s, const char *&errpos, string *err, char end = 0);
PEGExpr readatomexpr(PackratParser*p, const char *&s, const char *&errpos, string *err) {
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
	if (!isalpha(*s)) {
		errpos = s;
		if (err)*err = "unexpected symbol `"s + *s + "`";
		return{};
	}
	const char *b = s;
	while (isalnum(*s))s++;
	int id = p->_en[string(b, s - b)];
	return pnonterm(id);
}

PEGExpr readpostfixexpr(PackratParser*p, const char *&s, const char *&errpos, string *err) {
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


PEGExpr readconcatexpr(PackratParser*p, const char *&s, const char *&errpos, string *err, char end) {
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
		res /= t;
	}
	return res;
}

PEGExpr readexpr(PackratParser*p, const char *&s, const char *&errpos, string *err, char end) {
	PEGExpr res = readconcatexpr(p, s, errpos, err, end);
	if (errpos)return res;
	while(*ws(s)=='/') {
		res /= readconcatexpr(p, s, errpos, err, end);
		if (errpos)return res;
	}
	if (*s != end) {
		errpos = s;
		if(err)*err = (end ? "'"s + end + "'" : "end of parsing expression"s) + " expected";
		return{};
	}
	return res;
}

PEGExpr readexpr(PackratParser*p, const string & s, int *errpos, string * err) {
	const char * ep=0, *ps = s.c_str();
	PEGExpr r = readexpr(p, ps, ep, err);
	if (ep) {
		if (errpos)*errpos = ep - s.c_str();
		return{};
	}
	if (errpos)*errpos = -1;
	return r;
}
