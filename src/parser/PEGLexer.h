#pragma once
#include <utility>
#include <vector>
#include <regex>
#include <functional>
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
    Location() = default;
    explicit Location(Pos b): beg(b), end(b) {}
    Location(Pos b, Pos e): beg(b), end(e) {}
};

class LexIterator;
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
	// string text;
	PEGGrammar peg;
	// PackratParser packrat;
	// vector<pair<unique_ptr<regex>, int>> ncterms;
	vector<pair<int,int>> tokens; // Внутренний номер токена -> (номер packrat, внешний номер)
	Enumerator<string, unordered_map> _ten; // Внутренняя нумерация неконстантных токенов
	unordered_map<int, int> _intnum;
	TrieM<int> cterms;
	vector<string> ctokens;
	//int token(const string &x)const { return _ten.num(x); }
	//const string& token(int n)const { return _ten[n]; }
	vector<pair<int, int>> addedNcTokens;
	vector<int> specPseudonyms;
	// vector<pair<int, string>> addedCTokens;
	unordered_map<int, int> _counter; // !!!!!! Для отладки !!!!!!
	int ws_token=-1;
	int indent = -1, dedent = -1, check_indent = -1; // Токены для отслеживания отступов: indent -- увеличить отступ, dedent -- уменьшить отступ, check_indent -- проверить отступ
	int eol = -1, sof = -1, eof = -1; // Токены для конца строки и начала и конца файла
	NTSet special;   // Имеющиеся специальные токены (отступы, начала / концы строк)
	NTSet composite; // Составные токены
	NTSet simple;    // Простые токены (не являющиеся составными)

	PEGLexer() { 
		//_ten[""]; // Резервируем нулевой номер токена, чтобы номер любого токена был отличен от нуля
	}

	const std::string& tName(int intnum) const {
		return _ten[intnum];
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
    int declareSOFToken(const std::string &nm, int ext_num) {
        return _declareSpecToken(nm, ext_num, &sof, "SOF");
    }
	void addPEGRule(const string &nt, const string &rhs, int ext_num, bool to_begin = false) {
		int errpos = -1;
		string err;
		PEGExpr e = readParsingExpr(&peg, rhs, &errpos, &err);
		if (errpos >= 0)
			throw SyntaxError("Cannot parse PEG rule `"s + rhs + "` at position "+to_string(errpos)+": "+err);
		peg.add_rule(nt, e, to_begin);
		if (ext_num)declareNCToken(nt,ext_num);
	}
	int declareNCToken(const string& nm, int num, bool spec = false) {
		int t = _ten[nm];
		int a = peg._en.num(nm);
		if (!spec && a < 0)throw Exception("Cannot declare token `" + nm + "` when no rules exists");
		if (t >= (int)tokens.size())
			tokens.resize(t+1,make_pair(-1,-1));
		tokens[t] = make_pair(a,num);
		simple.add(t);
		return _intnum[num] = t;
	}
	int declareCompToken(const vector<pair<bool,string>>&toks, int num);
	int internalNum(const string& nm) {
		return _ten[nm];
	}
	int internalNum(int ext_num) {
		return _intnum.find(ext_num)->second;
	}
    int internalNumCheck(const string& nm) const {
	    return _ten.num(nm);
	}
	void addCToken(int t, const string &x) {
		cterms[x.c_str()] = t;
		if (ctokens.size() <= t)
			ctokens.resize(t + 1);
		ctokens[t] = x;
		//if (!curr.atEnd()) {
		//	addedCTokens.emplace_back(curr.pos, x);
		//}
	}
	int setWsToken(const string& tname) {
		return ws_token = peg._en[tname];
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
	// int *_pos = 0;
	LexIterator *curr = 0;
    int pos() const;
    Pos cpos() const;
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
	// iterator curr;
};

using SpecialLexerAction = std::function<int(PEGLexer*, const char* str, int &pos)>;

class LexIterator {
    struct IndentSt {
        bool fix = true;      // Была ли уже строка, где величина отступа зафиксирована
        int line = -1;       // Строка, где начался блок с отступом
        int start_col = -1; // Столбец, в котором было прочитано увеличение отступа
        int col = -1;      // Величина отступа
    };

    //vector<regex_iterator<const char*>> rit;
    //regex_iterator<const char*> rend;
    struct StAction {
        enum Type {
            Push,
            Pop,
            Change
        } type;
        IndentSt data;
    };
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
    PackratParser packrat;
    vector<IndentSt> indents;
    vector<StAction> undo_stack;
    PEGLexer *lex = nullptr;
    const char *s = nullptr;
    Pos cpos, cprev;
    bool rdws = true;
    int nlines = 0;
    int pos = 0;
    LexIterator *old_it = nullptr;
    bool _accepted = true;
    bool _at_end = false;
    bool _at_start = true;
    struct State {
        int nlines = 0;
        int pos = 0;
        bool rdws = true;
        Pos cpos, cprev;
        bool at_end = true;
        bool at_start = true;
    };
    [[nodiscard]] State state()const {
        State res;
        res.nlines = nlines;
        res.pos = pos;
        res.cpos = cpos;
        res.cprev = cprev;
        res.rdws = rdws;
        res.at_end = _at_end;
        res.at_start = _at_start;
        return res;
    }
    void restoreState(const State &st) {
        nlines  = st.nlines;
        pos     = st.pos;
        cpos    = st.cpos;
        cprev   = st.cprev;
        rdws    = st.rdws;
        _at_end = st.at_end;
        _at_start = st.at_start;
    }

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
    // LexIterator() = default;

    void readToken(const NTSet *t = 0) {
        Assert(_accepted);
        readToken_p(t);
        _accepted = false;
    }
    bool trySOF(Token *res) { // начало файла
        if (!_at_start) return false;
        *res = Token(lex->tokens[lex->sof].second, Location{cpos}, emptystr(pos), Token::Special);
        _at_start = false;
        return true;
    }
    bool tryEOL(Token *res) {
        // Переход на новую строку (считается, если после чтения пробелов/комментариев привело к увеличению номера строки
        if (cpos.line <= cprev.line + nlines) return false;
        *res = {lex->tokens[lex->eol].second, Location{cpos}, emptystr(pos), Token::Special};
        nlines++;
        return true;
    }
    bool tryIndent(Token *res) {
        // Если увеличение отступа допустимо, то оно читается вне зависимости от того, что дальше
        // Запрещено читать 2 раза подряд увеличение отступа с одной и той же позиции
        // иначе может произойти зацикливание, если неправильная грамматика, например, есть правило A -> indent A ...
        if (!s[pos] || (indents.back().col == cprev.col && indents.back().line == cprev.line)) return false;
        push_indent(IndentSt{ false, cprev.line, cprev.col, indents.back().col + 1 });
        *res = {lex->tokens[lex->indent].second, Location{cprev}, emptystr(pos), Token::Special};
        return true;
    }
    bool tryDedent(Token *res) {  // Уменьшение отступа
        if (cpos.col >= indents.back().col) return false;
        pop_indent();
        if (!indents.back().fix) {
            auto st = indents.back();
            st.fix = true;
            st.col = max(st.col, cpos.col);
            change_indent(st);
        }
        *res = {lex->tokens[lex->dedent].second, Location{cpos}, emptystr(pos), Token::Special};
        return true;
    }
    bool tryCheckIndent(Token *res)
    { // Проверка отступа
        if (!s[pos])
            return false;
        auto b = indents.back();
        if (cpos.col < b.col)
            return false;
        if (cpos.line > b.line) {
            if (!b.fix) {
                b.fix = true;
                b.col = cpos.col;
                change_indent(b);
            }
            if (b.col != cpos.col)
                return false;
        } else if(b.start_col != cprev.col) // Текст начался в той же строке, что и блок с отступами
            return false;

        *res = Token(lex->tokens[lex->check_indent].second, Location{cpos}, emptystr(pos), Token::Special);
        return true;
    }
    bool tryEOF(Token *res) {
        if (s[pos]) return false;
        *res = Token(lex->tokens[lex->eof].second, Location{cpos}, emptystr(pos), Token::Special);
        return true;
    }
    bool tryNCToken(int t, Token *res) {
        if (try_first) {
            int p = pos, tk = try_first(lex, s, p);
            if (tk >= 0) {
                if (t != tk) return false;
                *res = Token(lex->tokens[t].second, Location{ cpos }, substr(pos, p - pos), Token::NonConst);
                return true;
            }
        }
        if (lex->special.has(t)) {
            if (t == lex->sof && trySOF(res)) return true;
            if (t == lex->eol && tryEOL(res)) return true;
            if (t == lex->indent && tryIndent(res)) return true;
            if (t == lex->dedent && tryDedent(res)) return true;
            if (t == lex->check_indent && tryCheckIndent(res)) return true;
            if (t == lex->eof && tryEOF(res)) return true;
            return false;
        }
        int end = 0;
        if (packrat.parse(lex->tokens[t].first, pos, end, nullptr)) {
            *res = Token(lex->tokens[t].second, Location{cpos}, substr(pos, end - pos), Token::NonConst);
            rdws = true;
            return true;
        }
        return false;
    }
    void readWs() {
        if (lex->ws_token >= 0 && rdws) {
            cprev = cpos;
            int end = 0;
            if (packrat.parse(lex->ws_token, pos, end, 0) && end > pos)
                shift(end - pos);
            rdws = false; nlines = 0;
            if (!s[pos] && cpos.col > 1) {
                cpos.line++;
                cpos.col = 1; // Для корректного завершения блока отступов в конце файла виртуально добавляем пустую строку в конец
            }
        }
    }
    Substr substr(int position, int len) const{ return Substr{s + position, len}; }
    Substr emptystr(int position) const { return Substr{s + position, 0}; }
    bool readComposite(int i, Token &res)
    {
        for (auto[nc, tok] : lex->compTokens[i].t) {
            readWs();
            if (try_first) {
                int p=pos, t = try_first(lex, s, p);
                if (t >= 0) {
                    rdws = true;
                    if (t != tok) return false;
                    pos = p;
                	continue;
                }
            }
            if (nc) {
                if (!tryNCToken(tok, &res))
                    return false;
            } else {
                const int *n = lex->cterms(s, pos);
                if (!n || *n != tok)
                    return false;
                rdws = true;
            }
        }
        return true;
    }
    bool tryFirstAction(const NTSet &t);
    void readToken(const NTSet &t);
    void outputToken(int tok_id, Location loc, Substr str, Token::TType type) {
        curr_t.emplace_back(lex->tokens[tok_id].second, loc, str, type);
    }
    bool readSpecToken(const NTSet *t);
    void readToken_p(const NTSet *t);
    string genErrorMsg(const NTSet *t, int bpos);

    SpecialLexerAction try_first;
public:
    LexIterator(PEGLexer *l, std::string text, const NTSet *t=nullptr, SpecialLexerAction a={})
        : packrat(&l->peg, std::move(text)), lex(l), try_first(std::move(a)) {
        old_it = l->curr;
        l->curr = this;
        s = packrat.text.c_str();
        indents = { IndentSt{false,1,1,1} };
        cprev.line = 0;
        if(t) start(t);
        //for (auto &p : lex->ncterms)
        //	rit.push_back(regex_iterator<const char*>(s, s + text.size(), *p.first));
    }
    ~LexIterator() {
        lex->curr = old_it;
    }
    LexIterator(const LexIterator&) = delete;
    LexIterator& operator=(const LexIterator&) = delete;
    LexIterator(LexIterator&&) = delete;
    LexIterator& operator=(LexIterator&&) = delete;

    void setSpecialAction(const SpecialLexerAction &a) {
        try_first = a;
    }
    void start(const NTSet *t) {
        if(t)
            readToken(*t);
        else readToken(lex->simple);
    }
    bool atEnd() const { return _at_end; }
    void acceptToken(Token& tok);
    const vector<Token>& operator*()const {
        return curr_t;
    }
    LexIterator& operator++() {
        if(!_at_end)
            readToken();
        return *this;
    }
    /*LexIterator& go_next(const NTSet &t) {
        if (!_at_end)
            readToken(t);
        return *this;
    }*/
    bool go_next() {
        if (atEnd())return false;
        ++*this;
        return true;
    }
    bool go_next(const NTSet &t) {
        if (_at_end)return false;
        readToken(t);
        //go_next(t);
        return true;
    }
    const vector<Token> & tok() const {
        return **this;
    }
    int get_pos()const{ return pos; }
    Pos get_cpos()const { return cpos; }
    /*bool operator==(const iterator& it)const {
        return pos == it.pos;// curr.text.b == it.curr.text.b;
    }
    bool operator!=(const iterator& it)const {
        return pos != it.pos;// curr.text.b == it.curr.text.b;
        //return curr.text.b == it.curr.text.b;
    }*/
};

inline int PEGLexer::pos() const { return curr ? curr->get_pos() : 0; }
inline Pos PEGLexer::cpos() const { return curr ? curr->get_cpos() : Pos(); }
