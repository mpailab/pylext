#pragma once
#include <vector>
#include <regex>
#include <memory>
#include "Trie.h"
#include "Exception.h"
#include "common.h"
#include "PackratParser.h"
using namespace std;

struct Substr {
	const char *b = 0;
	int len = 0;
	Substr(const char *_b = 0, int _l = 0) :b(_b), len(_l) {}
};

struct Pos {
	int line = 1, col = 1;
	string str() const {
		return to_string(line) + ":" + to_string(col);
	}
};

struct Location {
	Pos beg, end;
};

struct Token {
	int type = 0, type2 = -1;
	Location loc;
	Substr text;
	string str()const { return string(text.b, text.len); }
	Token() {}
	Token(int t, Location l, Substr s) :type(t), loc(l), text(s) { loc.end.col--; }
};

struct PEGLexer {
	string text;
	PackratParser packrat;
	//vector<pair<unique_ptr<regex>, int>> ncterms;
	vector<pair<int,int>> tokens; // ¬нутренний номер токена -> (номер packrat, внешний номер)
	Enumerator<string, unordered_map> _ten; // ¬нутренн€€ нумераци€ неконстантных токенов
	TrieM<int> cterms;
	//int token(const string &x)const { return _ten.num(x); }
	//const string& token(int n)const { return _ten[n]; }
	vector<pair<int, int>> addedNcTokens;
	vector<pair<int, string>> addedCTokens;
	int ws_token=-1;
	PEGLexer() { 
		//_ten[""]; // –езервируем нулевой номер токена, чтобы номер любого токена был отличен от нул€
	}

	struct iterator {
		//vector<regex_iterator<const char*>> rit;
		//regex_iterator<const char*> rend;
		PEGLexer *lex = 0;
		const char *s = 0;
		Pos cpos;
		int pos = 0;
		
		bool atEnd() const { return !curr.text.b; }
		Token curr;
		void shift(int d) {
			for (; s[pos] && d; d--, pos++)
				if (s[pos] == '\n') {
					cpos.line++;
					cpos.col = 1;
				} else cpos.col++;
		}
		Pos shifted(int p) {
			Pos c = cpos;
			for (int q = pos; s[q]&&q<p; q++)
				if (s[q] == '\n') {
					c.line++;
					c.col = 1;
				} else c.col++;
			return c;
		}
		iterator() = default;
		iterator(PEGLexer *l) :lex(l), s(l->text.c_str()) {
			//for (auto &p : lex->ncterms)
			//	rit.push_back(regex_iterator<const char*>(s, s + text.size(), *p.first));
			readToken();
		}
		void readToken() {
			if (lex->ws_token >= 0) {
				int end = 0;
				if(lex->packrat.parse(lex->ws_token, pos, end, 0) && end>pos)
					shift(end - pos);
			}
			//while (isspace(s[pos]))
				//shift(1);
			if (!s[pos]) {
				curr = Token{ 0,{ cpos,cpos }, Substr() };
				return;
			}
			int p0 = pos, bpos = pos;
			const int* n = lex->cterms(s, p0);
			int m = 0, end = 0;
			int imax = -1;
			int best = -1, b1 = -1;
			for (int ni = 0; ni < (int)lex->tokens.size(); ni++) {
				if (!lex->packrat.parse(lex->tokens[ni].first, pos, end, 0)) continue;
				if (end > imax) {
					best = ni;
					m = 1;
					imax = end;
				} else if (end == imax) {
					m++; b1 = ni;
				}
			}
			if (n && p0 >= imax) {
				auto beg = cpos;
				shift(p0 - pos);
				curr = Token{ *n,{ beg,cpos }, Substr{ s + bpos, p0 - bpos } };
				if (p0 == imax && best >= 0 && m<2)
					curr.type2 = lex->tokens[best].second;
			} else {
				if (best < 0) {
					Pos ccpos = shifted(lex->packrat.errpos);
					string msg = "Unknown token at " + to_string(cpos.line) + ":" + to_string(cpos.col) + " : '" + string(s + bpos, strcspn(s + bpos, "\n")) + "',\n"
						"PEG error at " + ccpos.str() + " expected one of: " + lex->packrat.err_variants();
					throw SyntaxError(msg);
				} else if (m > 1) {
					throw SyntaxError("Lexer conflict: `" + string(s + bpos, imax - bpos) + "` may be 2 different tokens: "+lex->_ten[best]+" or "+lex->_ten[b1]);
				}
				auto beg = cpos;
				shift(end - bpos);
				curr = { lex->tokens[best].second, { beg,cpos }, Substr{ s + bpos, end - bpos } };
			}
		}
		const Token& operator*()const {
			return curr;
		}
		iterator& operator++() {
			readToken();
			return *this;
		}
		bool operator==(const iterator& it)const {
			return curr.text.b == it.curr.text.b;
		}
		bool operator!=(const iterator& it)const {
			return curr.text.b == it.curr.text.b;
		}
	};
	iterator begin() {
		return iterator(this);
	}
	iterator end()const { return iterator(); }
	struct Tokens {
		PEGLexer *lex;
		const std::string &text;
		Tokens(PEGLexer *l, const std::string &t) :lex(l), text(t) {}
		iterator begin() const {
			return PEGLexer::iterator(lex);
		}
		iterator end()const { return iterator(); }
	};
	/*Tokens tokens(const std::string &text) {
		return Tokens(this, text);
	}*/
	void start(const std::string &s = "") {
		if (s.size())
			text = s;
		packrat.setText(text);
		curr = begin();
	}
	bool go_next() {
		if (curr.atEnd())return false;
		++curr;
		return true;
	}
	bool atEnd() const {
		return curr.atEnd();
	}
	const Token & tok() {
		return *curr;
	}
	void addPEGRule(const string &nt, const string &rhs, int ext_num, bool to_begin = false) {
		int errpos = -1;
		string err;
		PEGExpr e = readParsingExpr(&packrat, rhs, &errpos, &err);
		if (errpos >= 0)
			throw SyntaxError("Cannot parse PEG rule `"s + rhs + "` at position "+to_string(errpos)+": "+err);
		packrat.add_rule(nt, e, to_begin);
		if (ext_num)declareNCToken(nt,ext_num);
	}
	void declareNCToken(const string& nm, int num) {
		int t = _ten[nm];
		int a = packrat._en.num(nm);
		if (a < 0)throw Exception("Cannot declare token `" + nm + "` when no rules exists");
		if (t >= (int)tokens.size())
			tokens.resize(t+1,make_pair(-1,-1));
		tokens[t] = make_pair(a,num);
	}
	void addCToken(int t, const string &x) {
		cterms[x.c_str()] = t;
		if (!curr.atEnd()) {
			addedCTokens.push_back(make_pair(curr.pos, x));
		}
	}
	int setWsToken(string tname) {
		return ws_token = packrat._en[tname];
	}
	/*void delTokens(int p, bool remove) {
		if (addedNcTokens.empty())return;
		int i, j;
		for (i = (int)addedNcTokens.size(); i--;)
			if (addedNcTokens[i].first < p)break;
		for (j = (int)addedCTokens.size(); i--;)
			if (addedCTokens[i].first < p)break;
		if (remove) {
			int e = addedNcTokens[i + 1].second;
			ncterms.resize(e);
			if (!curr.atEnd())
				curr.rit.resize(e);
			addedNcTokens.resize(i + 1);
			//TODO: –еализовать корректное удаление константных терминалов из Trie
		} else addedNcTokens.resize(i + 2);
	}*/
	iterator curr;
};
