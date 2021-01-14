#pragma once
#include <vector>
#include <regex>
#include <memory>
#include "Trie.h"
#include "Exception.h"
#include "common.h"
#include "NTSet.h"
#include "PackratParser.h"
#include "format.h"
using namespace std;

struct Substr {
	const char *b = 0;
	int len = 0;
	explicit Substr(const char *_b = 0, int _l = 0) :b(_b), len(_l) {}
};

struct Pos {
	int line = 1, col = 1;
	[[nodiscard]] string str() const {
		return to_string(line) + ":" + to_string(col);
	}
	bool operator==(const Pos &p) const { return line == p.line&&col == p.col; }
	bool operator!=(const Pos &p) const { return line != p.line||col != p.col; }
};

inline void print_formatted(string& buf, const Substr& s, char){ buf.append(s.b, s.len); }
inline void print_formatted(string& buf, const Pos& p, char){ buf += p.str(); }

struct Location {
	Pos beg, end;
};

struct Token {
	enum TType {
		Const=0,
		Special,
		NonConst,
		Composite
	};
	int type = 0, type2 = -1;
	Location loc;
	Substr text;
	TType nonconst = NonConst;
	[[nodiscard]] string str()const { return string(text.b, text.len); }
	[[nodiscard]] string short_str()const {
		if (text.len <= 80)return str();
		int n1 = 0,n2=text.len-1;
		for (; n1 < 60 && text.b[n1] && text.b[n1] != '\n';)n1++;
		for (; text.len-n2 < 75-n1 && text.b[n2]!='\n' && text.b[n2];)n2--;
		return string(text.b, n1)+" <...> "+string(text.b+n2+1,text.len-n2-1); 
	}
	Token() = default;
	Token(int t, Location l, Substr s, TType nc) :type(t), loc(l), text(s), nonconst(nc) { loc.end.col--; }
};

struct PEGLexer {
	struct IndentSt {
		bool fix = true;      // Была ли уже строка, где величина отступа зафиксирована
		int line = -1;       // Строка, где начался блок с отступом
		int start_col = -1; // Столбец, в котором было прочитано увеличение отступа
		int col = -1;      // Величина отступа
	};
	struct CompositeToken {
		vector<pair<bool,int>> t; // Номера токенов, из которых состоит составной токен
	};
	vector<CompositeToken> compTokens;
	int cmpcomp(int i1, int i2) {
		int l1 = (int)compTokens[i1].t.size(), l2 = (int)compTokens[i2].t.size(), l = min(l1, l2), r=0;
		for (int i = 0; i < l; i++) {
			auto [nc1, tok1] = compTokens[i1].t[i];
			auto [nc2, tok2] = compTokens[i2].t[i];
			int s1 = nc1 && special.has(compTokens[i1].t[i].second), s2 = nc2 && special.has(compTokens[i2].t[i].second);
			if (s1 != s2)return 2*(s1 - s2);
			if (!r && nc1 != nc2)r = nc1 ? 1 : -1;
		}
		if (l1 != l2)return l1 > l2 ? 1 : -1;
		return r;
	}
	string text;
	PackratParser packrat;
	//vector<pair<unique_ptr<regex>, int>> ncterms;
	vector<pair<int,int>> tokens; // Внутренний номер токена -> (номер packrat, внешний номер)
	Enumerator<string, unordered_map> _ten; // Внутренняя нумерация неконстантных токенов
	unordered_map<int, int> _intnum;
	TrieM<int> cterms;
	vector<string> ctokens;
	//int token(const string &x)const { return _ten.num(x); }
	//const string& token(int n)const { return _ten[n]; }
	vector<pair<int, int>> addedNcTokens;
	vector<pair<int, string>> addedCTokens;
	unordered_map<int, int> _counter; // !!!!!! Для отладки !!!!!!
	int ws_token=-1;
	int indent = -1, dedent = -1, check_indent = -1; // Токены для отслеживания отступов: indent -- увеличить отступ, dedent -- уменьшить отступ, check_indent -- проверить отступ
	int eol = -1, eof = -1; // Токены для конца строки и конца файла
	NTSet special;   // Имеющиеся специальные токены (отступы, начала / концы строк)
	NTSet composite; // Составные токены
	NTSet simple;    // Простые токены (не являющиеся составными)

	PEGLexer() { 
		//_ten[""]; // Резервируем нулевой номер токена, чтобы номер любого токена был отличен от нуля
	}

	struct iterator {
		//vector<regex_iterator<const char*>> rit;
		//regex_iterator<const char*> rend;
		vector<IndentSt> indents;
		struct StAction {
			enum Type {
				Push,
				Pop,
				Change
			} type;
			IndentSt data;
		};
		vector<StAction> undo_stack;
		void undo(const StAction &a) {
			switch (a.type) {
			case StAction::Push:
				indents.pop_back();
				break;
			case StAction::Pop:
				indents.push_back(a.data);
				break;
			case StAction::Change:
				indents.back() = a.data;
			}
		}
		void undo_all() {
			for (int i = (int)undo_stack.size(); i--;)
				undo(undo_stack[i]);
			undo_stack.clear();
		}

		IndentSt pop_indent() {
			undo_stack.push_back(StAction{ StAction::Pop, indents.back() });
			indents.pop_back();
			return undo_stack.back().data;
		}
		void push_indent(const IndentSt &st) {
			undo_stack.push_back(StAction{ StAction::Push, IndentSt() });
			indents.push_back(st);
		}
		void change_indent(const IndentSt &st) {
			undo_stack.push_back(StAction{ StAction::Change, indents.back() });
			indents.back() = st;
		}
		PEGLexer *lex = 0;
		const char *s = 0;
		Pos cpos, cprev;
		bool rdws = true;
		int nlines = 0;
		int pos = 0;
		bool _accepted = true;
		bool _at_end = true;
		struct State {
			int nlines = 0;
			int pos = 0;
			bool rdws = true;
			Pos cpos, cprev;
			bool at_end = true;
		};
		[[nodiscard]] State state()const {
			State res;
			res.nlines = nlines;
			res.pos = pos;
			res.cpos = cpos;
			res.cprev = cprev;
			res.rdws = rdws;
			res.at_end = _at_end;
			return res;
		}
		void restoreState(const State &st) {
			nlines  = st.nlines;
			pos     = st.pos;
			cpos    = st.cpos;
			cprev   = st.cprev;
			rdws    = st.rdws;
			_at_end = st.at_end;
		}

		[[nodiscard]] bool atEnd() const { return _at_end;/* !lex || pos >= (int)lex->text.size();curr.text.b;*/ }
		vector<Token> curr_t;
		void shift(int d) {
			for (; s[pos] && d; d--, pos++)
				if (s[pos] == '\n') {
					cpos.line++;
					cpos.col = 1;
				} else cpos.col++;
		}
		[[nodiscard]] Pos shifted(int p) const {
			Pos c = cpos;
			for (int q = pos; s[q]&&q<p; q++)
				if (s[q] == '\n') {
					c.line++;
					c.col = 1;
				} else c.col++;
			return c;
		}
		iterator() = default;
		explicit iterator(PEGLexer *l, const NTSet *t=0) :lex(l), s(l->text.c_str()) {
			_at_end = false;
			indents = { IndentSt{false,1,1,1} };
			cprev.line = 0;
			//for (auto &p : lex->ncterms)
			//	rit.push_back(regex_iterator<const char*>(s, s + text.size(), *p.first));
			if(t)
				readToken(*t);
			else readToken(lex->simple);
		}
		void readToken(const NTSet *t = 0) {
			Assert(_accepted);
			readToken_p(t);
			_accepted = false;
		}
		bool tryNCToken(int t, Token *res) {
			if (lex->special.has(t)) {
				if (t==lex->eol){
					if (cpos.line > cprev.line + nlines && lex->eol >= 0 && t == lex->eol) { // Переход на новую строку (считается, если после чтения пробелов/комментариев привело к увеличению номера строки
						*res = Token(lex->tokens[lex->eol].second, { cpos,cpos }, Substr(s + pos, 0), Token::Special);
						nlines++;
						return true;
					}
				} else if (t==lex->indent) {
					if(s[pos] && lex->indent >= 0 // Если увеличение отступа допустимо, то оно читается вне зависимости от того, что дальше 
					          && (indents.back().col != cprev.col || indents.back().line != cprev.line)) {     // Запрещено читать 2 раза подряд увеличение отступа с одной и той же позиции
						push_indent(IndentSt{ false, cprev.line, cprev.col, indents.back().col + 1 }); // иначе может произойти зацикливание, если неправильная грамматика, например, есть правило A -> indent A ...
						*res = Token(lex->tokens[lex->indent].second, { cprev, cprev }, Substr(s + pos, 0), Token::Special);
						return true;
					}
				} else if (t == lex->dedent) {
					if (cpos.col < indents.back().col) { // Уменьшение отступа
						pop_indent();
						if (!indents.back().fix) {
							auto st = indents.back();
							st.fix = true;
							st.col = max(st.col, cpos.col);
							change_indent(st);
						}
						*res = Token(lex->tokens[lex->dedent].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special);
						return true;
					}
				} else if (t == lex->check_indent) {
					if (s[pos]) { // Проверка отступа
						auto b = indents.back();
						bool indented = false;
						if (cpos.col >= b.col) {
							if (cpos.line > b.line) {
								if (!b.fix) {
									b.fix = true, b.col = cpos.col;
									change_indent(b);
								}
								if (b.col == cpos.col) indented = true;
							} else indented = (b.start_col == cprev.col);  // Текст начался в той же строке, что и блок с отступами
							if (indented) {
								*res = Token(lex->tokens[lex->check_indent].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special);
								return true;
							}
						}
					}
				} else if (t == lex->eof) {
					if (!s[pos]) {
						*res = Token(lex->tokens[lex->eof].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special);
						return true;
					}
				}
				return false;
			}
			int end = 0;
			if (lex->packrat.parse(lex->tokens[t].first, pos, end, 0)) {
				*res = Token{ lex->tokens[t].second, { cpos, cpos }, Substr{ s + pos, end - pos }, Token::NonConst };
				rdws = true;
				return true;
			}
			return false;
		}
		void readWs() {
			if (lex->ws_token >= 0 && rdws) {
				cprev = cpos;
				int end = 0;
				if (lex->packrat.parse(lex->ws_token, pos, end, 0) && end > pos)
					shift(end - pos);
				rdws = false; nlines = 0;
				if (!s[pos] && cpos.col > 1) { cpos.line++; cpos.col = 1; } // Для корректного завершения блока отступов в конце файла виртуально добавляем пустую строку в конец
			}
		}
		void readToken(const NTSet &t) {
			Assert(_accepted);
			curr_t.clear();
			if (t.intersects(lex->composite)) {
				auto st = state();
				Token res, rr; 
				int bpos = -1, imax = -1, i1=-1, m = 0;
				for (int i : t & lex->composite) {
					bool ok = true;
					for (auto[nc, tok] : lex->compTokens[i].t) {
						readWs();
						if (nc) {
							if (!tryNCToken(tok, &res)) {
								ok = false;
								break;
							}
						} else {
							const int *n = lex->cterms(s, pos);
							if (!n || *n != tok) {
								ok = false;
								break;
							}
							rdws = true;
						}
					}
					if (ok) {
						int c = imax < 0 ? 2 : lex->cmpcomp(i, imax);
						if (c == 2 || (c>=0 && pos > bpos-c)) {
							imax = i;
							bpos = pos;
							m = 1;
							rr = Token(lex->tokens[i].second, Location{ st.cpos,cpos }, Substr{ s + st.pos,pos - st.pos }, Token::Composite);
						} else if (c == 0 && pos == bpos) {
							m++;
							i1 = i;
						}
					}
					undo_all();
					restoreState(st);
				}
				if (imax >= 0) {
					if (m > 1) {
						throw SyntaxError("Lexer conflict at " + cpos.str() + ": `" + string(s + bpos, imax - bpos) + "` may be 2 different tokens: " + lex->_ten[imax] + " or " + lex->_ten[i1]);
					}
					curr_t.push_back(rr); // Составной токен (если прочитан) имеет приоритет перед обычным. Поэтому в случае успеха сразу возвращаем результат
					_accepted = false;
					return;
				}
				//restoreState(st);
			}
			readWs();
			readToken_p(&t);
			_accepted = false;
		}
		void readToken_p(const NTSet *t/*, int pos*/) {
			curr_t.clear();
			//_accepted = false;
			bool spec = t && t->intersects(lex->special);

			//auto prev = cpos;
			if (spec) { // Чтение специальных токенов
				if (cpos.line > cprev.line + nlines && lex->eol >= 0 && t->has(lex->eol)) { // Переход на новую строку (считается, если после чтения пробелов/комментариев привело к увеличению номера строки
					curr_t.push_back(Token(lex->tokens[lex->eol].second, { cpos,cpos }, Substr(s + pos, 0), Token::Special));
					nlines++;
					return;
				}
				if (spec && s[pos] && lex->indent >= 0 && t->has(lex->indent)) { // Если увеличение отступа допустимо, то оно читается вне зависимости от того, что дальше 
					if (indents.back().col != cprev.col || indents.back().line != cprev.line) {     // Запрещено читать 2 раза подряд увеличение отступа с одной и той же позиции
						indents.push_back(IndentSt{ false, cprev.line, cprev.col, indents.back().col + 1 }); // иначе может произойти зацикливание, если неправильная грамматика, например, есть правило A -> indent A ...
						curr_t.push_back(Token(lex->tokens[lex->indent].second, { cprev, cprev }, Substr(s + pos, 0), Token::Special));
						return;
					}
				}
				if (cpos.col < indents.back().col && lex->dedent >= 0 && t->has(lex->dedent)) { // Уменьшение отступа
					indents.pop_back();
					if (!indents.back().fix) {
						indents.back().fix = true;
						indents.back().col = max(indents.back().col, cpos.col);
					}
					curr_t.push_back(Token(lex->tokens[lex->dedent].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special));
					return;
				}
				if (s[pos] && lex->check_indent >= 0 && t->has(lex->check_indent)) { // Проверка отступа
					auto &b = indents.back();
					bool indented = false;
					if (cpos.col >= b.col) {
						if (cpos.line > b.line) {
							if (!b.fix)
								b.fix = true,
								b.col = cpos.col;
							if (b.col == cpos.col) indented = true;
						} else indented = (b.start_col == cprev.col);  // Текст начался в той же строке, что и блок с отступами
						if (indented) {
							curr_t.push_back(Token(lex->tokens[lex->check_indent].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special));
							return;
						}
					}
				}
			}

			if (!s[pos]) {
				if(spec && lex->eof>=0 && t->has(lex->eof))
					curr_t.push_back(Token(lex->tokens[lex->eof].second, { cpos, cpos }, Substr(s + pos, 0), Token::Special));
				else _at_end = true;
				return;
			}
			int p0 = pos, bpos = pos;
			const int* n = lex->cterms(s, p0);
			int m = 0, end = 0;
			int imax = -1;
			int best = -1, b1 = -1;
			for (int ni : t ? *t & lex->simple : lex->simple) { //= 0; ni < (int)lex->tokens.size(); ni++) {
				if (/*(t && !t->has(ni))||*/lex->special.has(ni))continue;
				//lex->_counter[ni]++;
				if (!lex->packrat.parse(lex->tokens[ni].first, pos, end, 0)) continue;
				curr_t.push_back(Token{ lex->tokens[ni].second, { cpos, cpos }, Substr{ s + bpos, end - bpos }, Token::NonConst});
				if (end > imax) {
					best = ni;
					m = 1;
					imax = end;
				} else if (end == imax) {
					m++;
					b1 = ni;
				}
			}
			if (curr_t.size() > 1)sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
			if (n){
				if (p0 >= imax) {
					auto beg = cpos;
					//shift(p0 - pos);
					curr_t.insert(curr_t.begin(), Token{ *n,{ cpos,shifted(p0) }, Substr{ s + bpos, p0 - bpos }, Token::Const });
					//if (p0 == imax && best >= 0 && m<2)
					//	curr.type2 = lex->tokens[best].second;
				} else { 
					curr_t.push_back(Token{ *n,{ cpos,shifted(p0) }, Substr{ s + bpos, p0 - bpos }, Token::Const }); 
					sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
				}
			} 
			if(!n || p0 < imax) {
				if (best < 0) {
					Pos ccpos = shifted(lex->packrat.errpos);
					string msg = "Unknown token at " + to_string(cpos.line) + ":" + to_string(cpos.col) + " : '" + string(s + bpos, strcspn(s + bpos, "\n")) + "',\n";
					if (t) {
						//_accepted = true;
						lex->packrat.reseterr();
						readToken_p(0);
						if (!curr_t.empty()) {
							//_accepted = false;
							msg += "it may be ";
							bool fst = true;
							for (auto &w : curr_t) {
								if (!fst)msg += ", "; fst = false;
								msg += lex->_ten[lex->internalNum(w.type)];
							}
							msg += " but expected one of: "; fst = true;
							for (int i : *t) {
								if (!fst)msg += ", "; fst = false;
								msg += lex->_ten[i];
							}
						} else msg += "PEG error at " + ccpos.str() + " expected one of: " + lex->packrat.err_variants();
					} else msg+="PEG error at " + ccpos.str() + " expected one of: " + lex->packrat.err_variants();
					throw SyntaxError(msg);
				} else if (t && m > 1) {
					throw SyntaxError("Lexer conflict at " + cpos.str() + ": `" + string(s + bpos, imax - bpos) + "` may be 2 different tokens: "+lex->_ten[best]+" or "+lex->_ten[b1]);
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
			
			if (tok.nonconst == Token::Composite) {
				Token r, rr;
				for (auto[nc, tok] : lex->compTokens[lex->internalNum(tok.type)].t) {
					readWs();
					if (nc) {
						Assert(tryNCToken(tok, &r));
					} else {
						Assert(lex->cterms(s,pos));
						rdws = true;
					}
				}
				Assert(tok.text.b + tok.text.len == s + pos);
			} else {
				shift(int(tok.text.b - (s + pos)));
				tok.loc.beg = cpos;
				if (tok.type == lex->eof)_at_end = true;
				shift(tok.text.len);
			}
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
				readToken(t);
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
		if (!s.empty())
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
	const vector<Token> & tok() const {
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
		simple.add(t);
		return _intnum[num] = t;
	}
	int declareCompToken(const vector<pair<bool,string>>&toks, int num) {
		Assert(toks.size()>1);
		CompositeToken ct;
		for (auto &s : toks) {
			if (!s.first) {
				if (const int *n = cterms(s.second.c_str()))
					ct.t.emplace_back(false, *n);
				else throw Exception("Token `" + s.second + "` (part of composite token) not declared");
			} else {
				if (_ten.has(s.second))
					ct.t.emplace_back(true, _ten[s.second]);
				else throw Exception("Token `" + s.second + "` (part of composite token) not declared");
			}
		}
		string nm=toks[0].second;
		for (int i = 1; i < (int)toks.size(); i++)
			(nm += " + ") += toks[i].second;
		int t = _ten[nm];
		int a = packrat._en.num(nm);
		if (t >= (int)tokens.size())
			tokens.resize(t + 1, make_pair(-1, -1));
		tokens[t] = make_pair(a, num);
		if ((int)compTokens.size() <= t)
			compTokens.resize(t + 1);
		compTokens[t] = move(ct);
		composite.add(t);
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
		if (ctokens.size() <= t)
			ctokens.resize(t + 1);
		ctokens[t] = x;
		if (!curr.atEnd()) {
			addedCTokens.emplace_back(curr.pos, x);
		}
	}
	int setWsToken(const string& tname) {
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
			//TODO: Реализовать корректное удаление константных терминалов из Trie
		} else addedNcTokens.resize(i + 2);
	}*/
	iterator curr;
};
