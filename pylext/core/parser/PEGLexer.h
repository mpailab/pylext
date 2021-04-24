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
	const char *b = nullptr;
	int len = 0;
	explicit Substr(const char *_b = nullptr, int _l = 0) :b(_b), len(_l) {}
    char operator[](size_t i)const { return b[i]; }
};

struct Pos {
	int line = 1, col = 1;
	[[nodiscard]] string str() const {
		return to_string(line) + ":" + to_string(col);
	}
	bool operator==(const Pos &p) const { return line == p.line && col == p.col; }
	bool operator!=(const Pos &p) const { return line != p.line || col != p.col; }
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
/// Структура, описывающая токен
struct Token {
    /// Вид токена
	enum TType {
		Const=0,  ///< Константный токен (фиксированная последовательность символов)
		Special,  ///< Специальный токен
		NonConst, ///< Обычный токен
	};
	int type = 0; ///< Номер терминала, соответствующий этому токену
	Location loc; ///< Расположение в тексте
	Substr text;  ///< Строковое значение (в виде ссылки на исходный текст)
	TType nonconst = NonConst; //< Вид токена; TODO: Используется ли?

	/// Значение токена в виде std::string
	[[nodiscard]] string str()const { return string(text.b, text.len); }
	[[nodiscard]] string short_str()const {
	    constexpr int max_len = 80, prefix_len = 60;
		if (text.len <= max_len)return str();
		int n1 = 0,n2=text.len-1;
		for (; n1 < prefix_len && text[n1] && text[n1] != '\n';)n1++;
		for (; text.len-n2 < max_len-7-n1 && text[n2]!='\n' && text[n2];)n2--;
		return string(text.b, n1)+" <...> "+string(text.b+n2+1,text.len-n2-1); 
	}
	Token() = default;
	Token(int t, Location l, Substr s, TType nc) :type(t), loc(l), text(s), nonconst(nc) { loc.end.col--; }
};

/// Структура, описывающая грамматику лексера
/** Грамматика лексера состоит из следующих компонентов:
 *  1. Константные токены, представляющие собой фиксированную последовательность символов --
 *     ключевые слова, знаки арифметических операций и т.п.
 *  2. Специальные токены -- служебные токены, не связанные
 *     с какой-либо последовательностью символов входной последовательности.
 *     Допустимые специальные токены
 *     - начало/конец файла
 *     - конец строки
 *     - увеличение/уменьшение/проверка отступа
 *     В грамматике может быть активно подмножество этих токенов
 *  3. Обычные токены, каждый из которых задаётся своим выражэением в PEG-грамматике.
 *     В обычных лексерах используются регулярные выражения, здесь более широкий класс
 *     для возможности обработки вложенных комментариев и т.п.
 */
class PEGLexer {
    /// Структура, описывающая специальный токен (начало/конец файла, увеличене/уменьшение отступа и т.п.)
    struct SpecialToken {
        int num = -1; // TODO: Разобраться с кучей разных нумераций и упростить по возможности
        int ext_num = -1;
        bool enforce = false; /// Если true, то токен читается вне зависимости от того, входит он в текущее множество допустимых токенов или нет
        bool allow(const NTSet& s) const{ return num>=0 && (enforce || s.has(num)); }
        explicit SpecialToken(bool enforce_): enforce(enforce_){}
    };

    unordered_map<int, int> _counter; // !!!!!! Для отладки !!!!!!
    int                     ws_token = -1;         // White space token

    // Специальные токены
    SpecialToken            preindent{false}; // Увеличение отступа (читается, если допустимо)
    SpecialToken            indent{true}; // Увеличение отступа (читается всегда)
    SpecialToken            dedent{true}; // Уменьшение отступа (читается всегда)
    SpecialToken            check_indent{false }; // проверка отступа (читается, если допустимо)
    SpecialToken            eol{false};   // Конец строки (читается, если допустимо)
    SpecialToken            eof{true};    // Конец файла (читается всегда)
    SpecialToken            sof{true};    // Начало файла (читается всегда)

    int _declareSpecToken(const std::string &nm, int ext_num, SpecialToken *tok, const std::string& intname, int enforce);

    NTSet special;   // Множество номеров имеющихся специальных токенов (отступы, начала / концы строк)
    NTSet simple;    // Простые токены; TODO: Вроде туда входят вообще все неконстантные токены. Возможно, что-то можно упростить

    PEGGrammar peg; // PEG-грамматика, описывающая токены языка

    vector<pair<int,int>> tokens; // Внутренний номер токена -> (номер packrat, внешний номер)
    Enumerator<string, unordered_map> _ten; // Внутренняя нумерация неконстантных токенов
    unordered_map<int, int> _intnum; // Соответствие между нумерациями
    TrieM<int> cterms;               // Префиксное дерево константных токенов
    vector<string> ctokens;          // Список константных токенов

    LexIterator *curr = nullptr;

    friend class LexIterator;
public:
    int preindent_num()const {return preindent.num; }
    int indent_num()const {return indent.num; }
    int dedent_num()const {return dedent.num; }
    int check_indent_num()const {return check_indent.num; }
    int eol_num() const { return eol.num; }
    int sof_num() const { return sof.num; }
    int eof_num() const { return eof.num; }
    int ws_num() const {return ws_token; }

    bool is_special(int t) const { return special.has(t); }
    bool is_const_token(int t) const {
        return t < ctokens.size() && !ctokens[t].empty();
    }
    const std::string& const_token(int t)const{
        return ctokens[t];
    }
    int ctoken_num()const{ return len(cterms); }
    int nctoken_num()const {return len(tokens); }

    int ctoken_num(const char *p)const{
        auto* n = cterms(p);
        return n ? *n : -1;
    }

	PEGLexer() = default;

	const std::string& tName(int intnum) const {
		return _ten[intnum];
	}

	int declareIndentToken(const std::string &nm, int ext_num, int enforce=0) {
		return _declareSpecToken(nm, ext_num, &indent, "indent", enforce);
	}
	int declareDedentToken(const std::string &nm, int ext_num, int enforce=0) {
		return _declareSpecToken(nm, ext_num, &dedent, "dedent", enforce);
	}
	int declareCheckIndentToken(const std::string &nm, int ext_num, int enforce=0) {
		return _declareSpecToken(nm, ext_num, &check_indent, "check_indent", enforce);
	}
	int declareEOLToken(const std::string &nm, int ext_num, int enforce=0) {
		return _declareSpecToken(nm, ext_num, &eol, "EOL", enforce);
	}
	int declareEOFToken(const std::string &nm, int ext_num, int enforce=0) {
		return _declareSpecToken(nm, ext_num, &eof, "EOF", enforce);
	}
    int declareSOFToken(const std::string &nm, int ext_num, int enforce=0) {
        return _declareSpecToken(nm, ext_num, &sof, "SOF", enforce);
    }
	void addPEGRule(const string &nt, const string &rhs, int ext_num, bool to_begin = false);
	int declareNCToken(const string& nm, int ext_num, bool spec = false);

	int internalNum(const string& nm) {
		return _ten[nm];
	}
	int internalNum(int ext_num) const {
		return _intnum.find(ext_num)->second;
	}
    int internalNumCheck(const string& nm) const {
	    return _ten.num(nm);
	}
	void addCToken(int t, const string &x);
	//Устанавливает имя специального токена, означающего пробел/комментарий в PEG-грамматике
	int setWsToken(const string& tname) {
		return ws_token = peg._en[tname];
	}
	~PEGLexer();

    int pos() const;
    Pos cpos() const;
};

using SpecialLexerAction = std::function<int(PEGLexer*, const char* str, int &pos)>;

class LexIterator {
    struct IndentSt {
        bool fix = true;      // Была ли уже строка, где величина отступа зафиксирована
        int line = -1;       // Строка, где начался блок с отступом
        int start_col = -1; // Столбец, в котором было прочитано увеличение отступа
        int col = -1;      // Величина отступа
    };

    struct StAction {
        enum Type {
            Push,
            Pop,
            Change
        } type;
        IndentSt data;
    };
    void undo(const StAction &a);
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
    bool was_eol = false;
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

    void readToken(const NTSet *t = 0) {
        Assert(_accepted);
        readToken_p(t);
        _accepted = false;
    }
    void readWs() {
        if (lex->ws_token >= 0 && rdws) {
            was_eol = false;
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

    bool tryFirstAction(const NTSet &t);
    void readToken(const NTSet &t);
    void outputToken(int tok_id, Location loc, Substr str, Token::TType type) {
        curr_t.emplace_back(lex->tokens[tok_id].second, loc, str, type);
    }
    bool readSpecToken(const NTSet &t);
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
    void clearToken();
    const vector<Token>& operator*()const {
        return curr_t;
    }
    LexIterator& operator++() {
        if(!_at_end)
            readToken();
        return *this;
    }

    bool go_next() {
        if (atEnd())return false;
        ++*this;
        return true;
    }
    bool go_next(const NTSet &t) {
        if (_at_end)return false;
        readToken(t);
        return true;
    }
    const vector<Token> & tok() const {
        return **this;
    }
    int get_pos()const{ return pos; }
    Pos get_cpos()const { return cpos; }
};

inline int PEGLexer::pos() const { return curr ? curr->get_pos() : 0; }
inline Pos PEGLexer::cpos() const { return curr ? curr->get_cpos() : Pos(); }

