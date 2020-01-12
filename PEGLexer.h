#pragma once
#include <vector>
#include <regex>
#include <memory>
#include "Trie.h"
#include "Exception.h"
#include "common.h"
#include "NTSet.h"
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
	bool operator==(const Pos &p) { return line == p.line&&col == p.col; }
	bool operator!=(const Pos &p) { return line != p.line||col != p.col; }
};

struct Location {
	Pos beg, end;
};

struct Token {
	int type = 0, type2 = -1;
	Location loc;
	Substr text;
	int nonconst = true;
	string str()const { return string(text.b, text.len); }
	string short_str()const { 
		if (text.len <= 80)return str();
		int n1 = 0,n2=text.len-1;
		for (; n1 < 60 && text.b[n1] && text.b[n1] != '\n';)n1++;
		for (; text.len-n2 < 75-n1 && text.b[n2]!='\n' && text.b[n2];)n2--;
		return string(text.b, n1)+" <...> "+string(text.b+n2+1,text.len-n2-1); 
	}
	Token() {}
	Token(int t, Location l, Substr s, bool nc) :type(t), loc(l), text(s), nonconst(nc) { loc.end.col--; }
};

struct PEGLexer {
	struct IndentSt {
		bool fix = true; // Ѕыла ли уже строка, где величина отступа зафиксирована
		int line; // —трока, где началс€ блок с отступом
		int start_col; // —толбец, в котором было прочитано увеличение отступа
		int col;  // ¬еличина отступа
	};
	string text;
	PackratParser packrat;
	//vector<pair<unique_ptr<regex>, int>> ncterms;
	vector<pair<int,int>> tokens; // ¬нутренний номер токена -> (номер packrat, внешний номер)
	Enumerator<string, unordered_map> _ten; // ¬нутренн€€ нумераци€ неконстантных токенов
	unordered_map<int, int> _intnum;
	TrieM<int> cterms;
	//int token(const string &x)const { return _ten.num(x); }
	//const string& token(int n)const { return _ten[n]; }
	vector<pair<int, int>> addedNcTokens;
	vector<pair<int, string>> addedCTokens;
	unordered_map<int, int> _counter; // !!!!!! ƒл€ отладки !!!!!!
	int ws_token=-1;
	int indent = -1, dedent = -1, check_indent = -1; // “окены дл€ отслеживани€ отступов: indent -- увеличить отступ, dedent -- уменьшить отступ, check_indent -- проверить отступ
	int eol = -1, eof = -1; // “окены дл€ конца строки и конца файла
	NTSet special; // »меющиес€ специальные токены (отступы, начала / концы строк)

	PEGLexer() { 
		//_ten[""]; // –езервируем нулевой номер токена, чтобы номер любого токена был отличен от нул€
	}

	struct iterator {
		//vector<regex_iterator<const char*>> rit;
		//regex_iterator<const char*> rend;
		vector<IndentSt> indents;
		PEGLexer *lex = 0;
		const char *s = 0;
		Pos cpos, cprev;
		bool rdws = true;
		int nlines = 0;
		int pos = 0;
		bool _accepted = true;
		bool _at_end = true;
		
		bool atEnd() const { return _at_end;/* !lex || pos >= (int)lex->text.size();curr.text.b;*/ }
		vector<Token> curr_t;
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
		iterator(PEGLexer *l, const NTSet *t=0) :lex(l), s(l->text.c_str()) {
			_at_end = false;
			indents = { IndentSt{false,1,1,1} };
			cprev.line = 0;
			//for (auto &p : lex->ncterms)
			//	rit.push_back(regex_iterator<const char*>(s, s + text.size(), *p.first));
			readToken(t);
		}
		void readToken(const NTSet *t = 0) {
			Assert(_accepted);
			readToken_p(t);
			_accepted = false;
		}
		void readToken_p(const NTSet *t/*, int pos*/) {
			curr_t.clear();
			//_accepted = false;
			bool spec = t && t->intersects(lex->special);

			if (lex->ws_token >= 0 && rdws) {
				cprev = cpos;
				int end = 0;
				if (lex->packrat.parse(lex->ws_token, pos, end, 0) && end > pos)
					shift(end - pos);
				rdws = false; nlines = 0;
				if (!s[pos] && cpos.col>1) { cpos.line++; cpos.col = 1; } // ƒл€ корректного завершени€ блока отступов в конце файла виртуально добавл€ем пустую строку в конец
			}
			
			//auto prev = cpos;
			if (spec) { // „тение специальных токенов
				if (cpos.line > cprev.line + nlines && lex->eol >= 0 && t->has(lex->eol)) { // ѕереход на новую строку (считаетс€, если после чтени€ пробелов/комментариев привело к увеличению номера строки
					curr_t.push_back(Token(lex->tokens[lex->eol].second, { cpos,cpos }, Substr(s + pos, 0), false));
					nlines++;
					return;
				}
				if (spec && s[pos] && lex->indent >= 0 && t->has(lex->indent)) { // ≈сли увеличение отступа допустимо, то оно читаетс€ вне зависимости от того, что дальше 
					if (indents.back().col != cprev.col || indents.back().line != cprev.line) {     // «апрещено читать 2 раза подр€д увеличение отступа с одной и той же позиции
						indents.push_back(IndentSt{ false, cprev.line, cprev.col, indents.back().col + 1 }); // иначе может произойти зацикливание, если неправильна€ грамматика, например, есть правило A -> indent A ...
						curr_t.push_back(Token(lex->tokens[lex->indent].second, { cprev, cprev }, Substr(s + pos, 0), false));
						return;
					}
				}
				if (cpos.col < indents.back().col && lex->dedent >= 0 && t->has(lex->dedent)) { // ”меньшение отступа
					indents.pop_back();
					if (!indents.back().fix) {
						indents.back().fix = true;
						indents.back().col = max(indents.back().col, cpos.col);
					}
					curr_t.push_back(Token(lex->tokens[lex->dedent].second, { cpos, cpos }, Substr(s + pos, 0), false));
					return;
				}
				if (s[pos] && lex->check_indent >= 0 && t->has(lex->check_indent)) { // ѕроверка отступа
					auto &b = indents.back();
					bool indented = false;
					if (cpos.col >= b.col) {
						if (cpos.line > b.line) {
							if (!b.fix)
								b.fix = true,
								b.col = cpos.col;
							if (b.col == cpos.col) indented = true;
						} else indented = (b.start_col == cprev.col);  // “екст началс€ в той же строке, что и блок с отступами
						if (indented) {
							curr_t.push_back(Token(lex->tokens[lex->check_indent].second, { cpos, cpos }, Substr(s + pos, 0), false));
							return;
						}
					}
				}
			}

			if (!s[pos]) {
				if(spec && lex->eof>=0 && t->has(lex->eof))
					curr_t.push_back(Token(lex->tokens[lex->eof].second, { cpos, cpos }, Substr(s + pos, 0), false));
				else _at_end = true;
				return;
			}
			int p0 = pos, bpos = pos;
			const int* n = lex->cterms(s, p0);
			int m = 0, end = 0;
			int imax = -1;
			int best = -1, b1 = -1;
			for (int ni = 0; ni < (int)lex->tokens.size(); ni++) {
				if ((t && !t->has(ni))||lex->special.has(ni))continue;
				//lex->_counter[ni]++;
				if (!lex->packrat.parse(lex->tokens[ni].first, pos, end, 0)) continue;
				curr_t.push_back(Token{ lex->tokens[ni].second, { cpos, cpos }, Substr{ s + bpos, end - bpos }, true});
				if (end > imax) {
					best = ni;
					m = 1;
					imax = end;
				} else if (end == imax) {
					m++;
				}
			}
			if (curr_t.size() > 1)sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
			if (n){
				if (p0 >= imax) {
					auto beg = cpos;
					//shift(p0 - pos);
					curr_t.insert(curr_t.begin(), Token{ *n,{ cpos,shifted(p0) }, Substr{ s + bpos, p0 - bpos }, false });
					//if (p0 == imax && best >= 0 && m<2)
					//	curr.type2 = lex->tokens[best].second;
				} else { 
					curr_t.push_back(Token{ *n,{ cpos,shifted(p0) }, Substr{ s + bpos, p0 - bpos }, false }); 
					sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
				}
			} 
			if(!n || p0 < imax) {
				if (best < 0) {
					Pos ccpos = shifted(lex->packrat.errpos);
					string msg = "Unknown token at " + to_string(cpos.line) + ":" + to_string(cpos.col) + " : '" + string(s + bpos, strcspn(s + bpos, "\n")) + "',\n";
					if (t) {
						//_accepted = true;
						readToken();
						//_accepted = false;
						msg += "it may be ";
						bool fst = true;
						for (auto &t : curr_t) {
							if (!fst)msg += ", "; fst = false;
							msg += lex->_ten[lex->internalNum(t.type)];
						}
						msg += " but expected one of: "; fst = true;
						for (int i : *t) {
							if (!fst)msg += ", "; fst = false;
							msg += lex->_ten[i];
						}
					} else msg+="PEG error at " + ccpos.str() + " expected one of: " + lex->packrat.err_variants();
					throw SyntaxError(msg);
				} else if (m > 1) {
					throw SyntaxError("Lexer conflict: `" + string(s + bpos, imax - bpos) + "` may be 2 different tokens: "+lex->_ten[best]+" or "+lex->_ten[b1]);
				}
				//auto beg = cpos;
				//shift(end - bpos);
				//curr = { lex->tokens[best].second, { beg,cpos }, Substr{ s + bpos, end - bpos } };
			}
			rdws = true;
			//_accepted = false;
		}
		void acceptToken(Token& tok) {
			if (_accepted) {
				Assert(curr_t[0].type==tok.type && curr_t[0].text.b == tok.text.b && curr_t[0].text.len == tok.text.len);
				return;
			}
			Assert(s + pos <= tok.text.b);
			shift(int(tok.text.b - (s+pos)));
			tok.loc.beg = cpos;
			if (tok.type == lex->eof)_at_end = true;
			shift(tok.text.len);
			tok.loc.end = cpos;
			curr_t.resize(1); curr_t[0] = tok;
			_accepted = true;
		}
		const vector<Token>& operator*()const {
			return curr_t;
		}
		iterator& operator++() {
			if(!_at_end)
				readToken();
			return *this;
		}
		iterator& go_next(const NTSet &t) {
			if (!_at_end)
				readToken(&t);
			return *this;
		}
		bool operator==(const iterator& it)const {
			return pos == it.pos;// curr.text.b == it.curr.text.b;
		}
		bool operator!=(const iterator& it)const {
			return pos != it.pos;// curr.text.b == it.curr.text.b;
			//return curr.text.b == it.curr.text.b;
		}
	};
	iterator begin(const NTSet *t=0) {
		return iterator(this,t);
	}
	iterator end()const { return iterator(); }
	/*struct Tokens {
		PEGLexer *lex;
		const std::string &text;
		Tokens(PEGLexer *l, const std::string &t) :lex(l), text(t) {}
		iterator begin() const {
			return PEGLexer::iterator(lex);
		}
		iterator end()const { return iterator(); }
	};*/
	/*Tokens tokens(const std::string &text) {
		return Tokens(this, text);
	}*/
	void start(const std::string &s = "", const NTSet* t = 0) {
		if (s.size())
			text = s;
		packrat.setText(text);
		curr = begin(t);
	}
	void acceptToken(Token& t) {
		curr.acceptToken(t);
	}
	bool go_next() {
		if (curr.atEnd())return false;
		++curr;
		return true;
	}
	bool go_next(const NTSet &t) {
		if (curr.atEnd())return false;
		curr.go_next(t);
		return true;
	}
	const std::string& tName(int intnum) const {
		return _ten[intnum];
	}
	bool atEnd() const {
		return curr.atEnd();
	}
	const vector<Token> & tok() {
		return *curr;
	}
	int _declareSpecToken(const std::string &nm, int ext_num, int *_pnum, const std::string& intname) {
		if (*_pnum >= 0) {
			if (_ten[*_pnum] == nm) return *_pnum;
			else throw GrammarError(intname + " token already declared with name `" + _ten[*_pnum] + "`");
		}
		*_pnum = declareNCToken(nm, ext_num, true);
		special.add(*_pnum);
		return *_pnum;
	}
	int declareIndentToken(const std::string &nm, int ext_num) {
		return _declareSpecToken(nm, ext_num, &indent, "indent");
	}
	int declareDedentToken(const std::string &nm, int ext_num) {
		return _declareSpecToken(nm, ext_num, &dedent, "dedent");
	}
	int declareCheckIndentToken(const std::string &nm, int ext_num) {
		return _declareSpecToken(nm, ext_num, &check_indent, "check_indent");
	}
	int declareEOLToken(const std::string &nm, int ext_num) {
		return _declareSpecToken(nm, ext_num, &eol, "EOL");
	}
	int declareEOFToken(const std::string &nm, int ext_num) {
		return _declareSpecToken(nm, ext_num, &eof, "EOF");
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
	int declareNCToken(const string& nm, int num, bool spec = false) {
		int t = _ten[nm];
		int a = packrat._en.num(nm);
		if (!spec && a < 0)throw Exception("Cannot declare token `" + nm + "` when no rules exists");
		if (t >= (int)tokens.size())
			tokens.resize(t+1,make_pair(-1,-1));
		tokens[t] = make_pair(a,num);
		return _intnum[num] = t;
	}
	int internalNum(const string& nm) {
		return _ten[nm];
	}
	int internalNum(int ext_num) {
		return _intnum.find(ext_num)->second;
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
	~PEGLexer() {
		if (!_counter.empty()) {
			cout << "\n============ LEXER STATS =============\n";
			for (auto &p : _counter) {
				cout << "  " << _ten[p.first] << ":\t" << p.second << "\n";
			}
			cout << "======================================\n";
		}
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
