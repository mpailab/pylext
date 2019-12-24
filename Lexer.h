#pragma once
#include <vector>
#include <regex>
#include <memory>
#include "Trie.h"
#include "Exception.h"
#include "common.h"
using namespace std;

struct Substr {
	const char *b=0;
	int len=0;
	Substr(const char *_b = 0, int _l = 0) :b(_b), len(_l) {}
};

struct Pos {
	int line=1, col=1;
};

struct Location {
	Pos beg, end;
};

struct Token {
	int type = 0;
	Location loc;
	Substr text;
	string str()const { return string(text.b, text.len); }
	Token() {}
	Token(int t, Location l, Substr s) :type(t), loc(l), text(s) { loc.end.col--; }
};

struct Lexer {
	string text;
	vector<pair<unique_ptr<regex>, int>> ncterms;
	TrieM<int> cterms;
	vector<pair<int, int>> addedNcTokens;
	vector<pair<int, string>> addedCTokens;
	struct iterator {
		vector<regex_iterator<const char*>> rit;
		regex_iterator<const char*> rend;
		const Lexer *lex = 0;
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
		iterator() = default;
		iterator(const Lexer *l, const std::string &text) :lex(l), s(text.c_str()) { 
			for (auto &p : lex->ncterms)
				rit.push_back(regex_iterator<const char*>(s, s + text.size(), *p.first));
			readToken();
		}
		void readToken() {
			while (isspace(s[pos]))
				shift(1);
			if (!s[pos]) {
				curr = Token{ 0, {cpos,cpos}, Substr() };
				return;
			}
			int p0 = pos, bpos = pos;
			if (const int *n = lex->cterms(s, p0)) {
				auto beg = cpos;
				shift(p0 - pos);
				curr = Token{ *n, {beg,cpos}, Substr{s + bpos, p0 - bpos} };
			} else {
				int m = 0;
				int imax = -1;
				const match_results<const char*> *best = 0;
				for (int ni = 0; ni < (int)rit.size(); ++ni){
					auto& i = rit[ni];
					while (i != rend && (*i)[0].first < s+pos)
						++i;
					if (i == rend || (*i)[0].first > s+pos)continue;
					if (!best||i->length() > best->length()) {
						best = &*i;
						m = 0;
						imax = ni;
					}
					if (i->length() == best->length())
						m++;
				}
				if (!best) {
					throw SyntaxError("Unknown token at " + to_string(cpos.line) + ":" + to_string(cpos.col) + " : '"+string(s+bpos,strcspn(s+bpos,"\n"))+"'");
				} else if (m > 1) {
					throw SyntaxError("Lexer conflict: `" + best->str()+"` may be 2 different tokens");
				}
				auto beg = cpos;
				shift((int)best->length());
				curr = { lex->ncterms[imax].second, {beg,cpos}, Substr{s + bpos, (int)best->length()} };
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
	iterator begin() const {
		return iterator(this,text);
	}
	iterator end()const { return iterator(); }
	struct Tokens {
		const Lexer *lex;
		const std::string &text;
		Tokens(const Lexer *l, const std::string &t) :lex(l),text(t) {}
		iterator begin() const {
			return Lexer::iterator(lex, text);
		}
		iterator end()const { return iterator(); }
	};
	Tokens tokens(const std::string &text)const {
		return Tokens(this, text);
	}
	void start(const std::string &s="") {
		if(s.size())text = s;
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
	void addNCToken(int t, const regex& re) {
		ncterms.push_back(make_pair(make_unique<regex>(re), t));
		if (!curr.atEnd()) {
			curr.rit.push_back(regex_iterator<const char*>(curr.s + curr.pos, curr.s + text.size(), *ncterms.back().first));
			addedNcTokens.push_back(make_pair(curr.pos, len(ncterms)-1));
		}
	}
	void addCToken(int t, const string &x) {
		cterms[x.c_str()] = t;
		if (!curr.atEnd()) {
			addedCTokens.push_back(make_pair(curr.pos, x));
		}
	}
	void delTokens(int p, bool remove) {
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
			//TODO: Реализовать корректное удаление константных терминалов из Trie
		} else addedNcTokens.resize(i + 2);
	}
	iterator curr;
};

#if 0
class Lexer {
	const char *w;
	int pos;
public:
	Lexer& space() { while (isspace(w[pos]))pos++; return *this; }
	char get() { return w[pos] ? w[pos++] : w[pos]; }
	char peek() { return w[pos]; }
	char operator*() { return w[pos]; }
	Lexer& operator++() { if (w[pos])pos++; return *this; }
	char operator[](int p) { return w[pos + p]; }
	void setstr(const char *x) { w = x; pos = 0; }
	bool starts(const Trie &x) {
		const Trie *curr = &x;
		int p = pos;
		while (w[p]) {//!curr->next.empty()) {
			if (curr->final)return true;
			if (curr->next.empty())return false;
			curr = &curr->next[(unsigned char)w[p++]];
		}
		return curr->final;
	}
	template<class T>
	T* starts(const TrieM<T> &x, int *ppos = 0) {
		const TrieM<T> *curr = &x;
		int p = pos;
		while (w[p]) {//!curr->next.empty()) {
			if (curr->final)return &curr->val;
			if (curr->next.empty())return 0;
			curr = &curr->next[(unsigned char)w[p++]];
		}
		if (ppos)*ppos = p;
		return curr->final ? &curr->val : 0;
	}
	const NTTreeNode* startsC(const NTTreeNode *x, NTSetS& s);
};
#endif
