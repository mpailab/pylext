#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <map>
#include <utility>
#include <vector>
#include <cctype>
#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include "Exception.h"
#include "PEGLexer.h"
#include "NTSet.h"
#include "Hash.h"
#include "Vector.h"
#include "Alloc.h"
#include "GCAlloc.h"

constexpr int DBG_SHIFT = 0x1;
constexpr int DBG_REDUCE = 0x2;
constexpr int DBG_STATE = 0x4;
constexpr int DBG_LOOKAHEAD = 0x8;
constexpr int DBG_TOKEN = 0x10;
constexpr int DBG_RULES = 0x20;
constexpr int DBG_QQ = 0x40;
constexpr int DBG_ALL = 0xFFFFFFF;

using namespace std;

class ParseNode;
struct ParseTree;

/// Узел дерева разбора
class ParseNode {
public:
    enum Type {
        Ordinary = 0,
        Final,
        FinalUsed
    };
    Type type = Ordinary;
    int used = 0, refs = 0;
    int nt = -1;          // Номер терминала или нетерминала
    int B = -1;           // Номер промежуточного нетерминала в случае свёртки nt <- B <- rule
    int rule=-1;          // Номер правила (-1 => терминал)
    int rule_id = -1;     // Внешний номер правила
    string term;          // Строковое значение (если терминал)
    int size = 1;
    unsigned lpr = -1, rpr = -1; // Левый и правый приоритеты (если unsigned(-1), то приоритеты не заданы)
    vector<ParseNode*> ch; // Дочерние узлы
    Location loc;         // Размещение фрагмента в тексте
    bool flattened = false;

    [[nodiscard]] bool isTerminal() const { return rule < 0; }
    int rule_nt() const { return B>=0 ? B : nt; }
    [[nodiscard]] bool haslpr()const { return (int)lpr != -1; }
    [[nodiscard]] bool hasrpr()const { return (int)rpr != -1; }
    ParseNode& operator[](size_t i) { return *ch[int(i)<0 ? i+ch.size() : i]; }
    const ParseNode& operator[](size_t i)const { return *ch[int(i)<0 ? i + ch.size() : i]; }
    ParseNode() = default;
    ParseNode(const ParseNode&) = delete;
    ParseNode& operator=(const ParseNode&) = delete;

    ParseNode(ParseNode&&) = default;
    void updateSize() {
        size = 1;
        for (auto *n : ch)
            size += n->size;
    }
    ParseNode* balancePr() {        // TODO: обавить проверку, что нет неоднозначности свёртки разных нетерминалов, которые не сравниваются по приоритетам                                                     
        if (haslpr() && B == ch[0]->B)                        /*        this                           r            */
            lpr = min(lpr, ch[0]->lpr);                       /*       / .. \                         / \           */
        if (hasrpr()) {                                       /*      x...   r                       .   .          */
            ParseNode *pn = ch.back(), *pp;                   /*            / \                     .     .         */
            if (pn->B == B && pn->lpr < rpr) {                /*           .   .     ==>           .........        */
                do {                                          /*          .     .                 /                 */
                    pn->lpr = min(pn->lpr, lpr);              /*         .........                pp                */
                    pp = pn; pn = pn->ch[0];                  /*        /                       / .. \              */
                } while (pn->B == B && pn->lpr < rpr);        /*      pp                      this ...y             */
                ParseNode *l = pp->ch[0], *r = ch.back();     /*    / .. \                   / .. \                 */
                pp->ch[0] = this;                             /*   pn  ...y                 x...  pn                */
                ch.back() = l;

                int root_nt = nt; // переставляем типы нетерминалов
                nt = l->nt;
                l->nt = r->nt;
                r->nt = root_nt;

                return r; // корнем становится r
            }
        }
        return this; // корень остаётся прежним
    }
    ParseNode& operator=(ParseNode&&) = default;
    ~ParseNode() = default;
    void serialize(vector<unsigned char>& res);
};

bool equal_subtrees(const ParseNode* x, const ParseNode* y);

using ParseNodePtr = GCPtr<ParseNode>;
class GrammarState;
struct SStack;
struct PStack;

class ParseErrorHandler {
public:
    struct Hint {
        enum Type {
            Uncorrectable,
            SkipT,
            SkipNT,
            InsertT,
            InsertNT
        } type = Uncorrectable;
        int added = 0;
    };
    virtual Hint onRRConflict(GrammarState* state, const SStack& ss, const PStack& sp, int term, int nt1, int nt2, int depth, int place);
    virtual Hint onNoShiftReduce(GrammarState* g, const SStack& s, const PStack& p, const Token& tok);
};


struct ParseTree { // TODO: Нужен ли вообще отдельный такой класс? По сути это то же самое, что ParseNodePtr
    ParseNodePtr root;
    ParseTree() = default;
    explicit ParseTree(ParseNode* pn): root(pn) {}
};

template<class T>
struct Ref : unique_ptr<T> {
    Ref() :unique_ptr<T>(new T) {}
    Ref(const Ref<T>&r): unique_ptr<T>(r ? new T(*r) : nullptr) {}
    Ref(Ref<T>&&)  noexcept = default;
    Ref<T>& operator= (const Ref<T>& r) {
        if(r) this->reset(new T(*r));
        else this->reset();
        return *this;
    }
    Ref<T>& operator=(Ref<T>&&)  noexcept = default;
    ~Ref() = default;
};

struct NTTreeNode {
    unordered_map<int, Ref<NTTreeNode>> termEdges;    // Ветвление по неконстантным терминалам
    unordered_map<int, Ref<NTTreeNode>> ntEdges;      // Ветвление по нетерминалам
    NTSet phi;                                 // Нетерминалы, по которым можно пройти до текущей вершины
    NTSet finalNT;                             // Нетерминалы, для которых текущая вершина -- финальная
    NTSet next;                                // Множество нетерминалов, по которым можно пройти дальше по какому-либо ребру
    NTSet nextnt;                              // Множество нетерминалов, по которым можно пройти дальше по нетерминальному ребру
    unordered_map<int, int> rules;             // Правила по номерам нетерминалов
    int pos = 0;                               // Расстояние до корня дерева
    int _frule = -1;

    ///////////////// Для вычисления множества допустимых терминалов ////////////////////////////
    unordered_map<int, NTSet> next_mt;           // Сопоставляет нетерминалу A множество терминалов, по которым можно пройти из данной вершины, читая правило для A
    unordered_map<int, NTSet> next_mnt;          // Сопоставляет нетерминалу A множество нетерминалов, по которым можно пройти из данной вершины, читая правило для A
    /////////////////////////////////////////////////////////////////////////////////////////////
    
    [[nodiscard]] const NTTreeNode* nextN(int A)const {
        auto it = ntEdges.find(A);
        if (it == ntEdges.end())return nullptr;
        return it->second.get();
    }
    [[nodiscard]] int rule(int A)const {
        return rules.find(A)->second;
    }
};

/// Набор вспомогательных таблиц для парсера
class TF {
public:
    vector<NTSet> T;      // По нетерминалу возвращает все нетерминалы, которые наследуются от данного нетерминала (A :-> {B | B => A})
    vector<NTSet> inv;    // По нетерминалу возвращает все нетерминалы, от которых наследуется данный нетерминал   (A :-> {B | A => B})
    vector<NTSet> fst;    // По нетерминалу возвращает все нетерминалы, с которых может начинаться данный нетерминал
    vector<NTSet> ifst;   // По нетерминалу возвращает все нетерминалы, которые могут начинаться с данного нетерминала

    vector<NTSet> fst_t;  // По нетерминалу возвращает все терминалы, с которых может начинаться данный нетерминал
    vector<NTSet> ifst_t; // По терминалу возвращает все нетерминалы, которые могут начинаться с данного терминала

    void addRuleBeg(int /*pos*/, int A, int rhs0, int len) { // Добавляет в структуру правило A -> rhs0 ...; pos -- позиция в тексте; len -- длина правой части правила
        int mx = max(A, rhs0);
        checkSize(mx);
        if (len == 1) {
            for(int x : inv[rhs0])
                T[x] |= T[A]; // B -> A ..., A -> rhs0 ... ==> B -> rhs0 ...

            for(int x : T[A])
                inv[x] |= inv[rhs0]; // A -> rhs0 ..., rhs0 -> B ... ==> A -> B ...
        }
        for (int x : ifst[A]) {
            fst[x] |= fst[rhs0];
            fst_t[x] |= fst_t[rhs0];
        }
        for (int x : fst_t[rhs0])
            ifst_t[x] |= ifst[A];
        for(int x : fst[rhs0])
            ifst[x] |= ifst[A];
        // TODO: как-то запоминать позицию и изменения, чтобы можно было потом откатить назад.
    }
    void addRuleBeg_t(int /*pos*/, int A, int rhs0) { // Добавляет в структуру правило A -> rhs0 ..., только здесь rhs0 -- терминал; pos -- позиция в тексте;
        checkSize(A);
        checkSize_t(rhs0);
        for (int x : ifst[A])
            fst_t[x].add(rhs0);
        ifst_t[rhs0] |= ifst[A];
        // TODO: как-то запоминать позицию и изменения, чтобы можно было потом откатить назад.
    }
    void checkSize(int A) {
        int n0 = (int)T.size();
        if (n0 <= A) {
            T.resize(A + 1), inv.resize(A + 1), fst.resize(A + 1), ifst.resize(A + 1), fst_t.resize(A + 1);
            for (int i = n0; i <= A; i++) {
                T[i].add(i);
                inv[i].add(i);
                fst[i].add(i);
                ifst[i].add(i);
            }
        }
    }
    void checkSize_t(int t) {
        int n0 = (int)ifst_t.size();
        if (n0 <= t) {
            ifst_t.resize(t + 1);
        }
    }
};

// Элемент правой части правила (терминал или нетерминал)
struct RuleElem {
    int num; // Номер
    bool cterm; // для терминала: является ли он константным
    bool term;  // true => терминал, false => нетерминал
    bool save;  // Следует ли данный элемент сохранять в дереве разбора; по умолчанию save = !cterm, поскольку только неконстантные элементы имеет смысл сохранять
};

class ParseContext;
class GrammarState;
using SemanticAction = function<void(ParseContext& g, ParseNodePtr&)>;

class CFGRule {
public:
    int A=-1; // Нетерминал в левой части правила
    vector<RuleElem> rhs; // Правая часть правила 
    int used = 0;
    SemanticAction action;
    int ext_id = -1;
    int lpr = -1, rpr = -1; // левый и правый приоритеты правила; у инфиксной операции задаются оба, у префиксной -- только правый, а у постфиксной -- только левый
};

template<class T>
class dvector {
    vector<T> v;
    int mx = -1;
public:
    const T& operator[](size_t i)const {
        return v[i];
    }
    T& operator[](size_t i) {
        mx = max(mx, (int)i);
        if (i >= v.size())v.resize(i + 1);
        return v[i];
    }
    [[nodiscard]] int size()const { return (int)v.size(); }
    void clear() {
        for (int i = 0; i <= mx; i++)v[i].clear();
        mx = -1;
    }
};

void setDebug(int b);

template<class T>
class CacheVec: public vector<unique_ptr<T>> {
public:
    CacheVec() = default;
    CacheVec(const CacheVec<T>& v) {}
    CacheVec(CacheVec<T>&&) noexcept = default;
    CacheVec& operator=(const CacheVec&) { return *this; }
    CacheVec& operator=(CacheVec&&) { return *this; }
    ~CacheVec() = default;
};

class GrammarState {
public:
    using NewNTAction = function<void(GrammarState*, const string&, int)>; // Обработчик события добавления нового нетерминала
    using NewTAction  = function<void(GrammarState*, const string&, int)>; // Обработчик события добавления н6ового терминала

    unordered_map<int, NTSet> tFirstMap;   // По терминалу возвращает, какие нетерминалы могут начинаться с данного терминала
    vector<vector<NTTreeNode*>> ntRules;   // Каждому нетерминалу сопоставляется список финальных вершин, соответствующих правилам для данного нетерминала
    NTTreeNode root;        // Корневая вершина дерева правил
    Enumerator<string, unordered_map> nts; // Нумерация нетерминалов
    Enumerator<string, unordered_map> ts;  // Нумерация терминалов
    unordered_map<string, int> _start_nt;
    vector<CFGRule> rules;
    std::map<pair<string, vector<string>>, int> rule_map;

    vector<NewNTAction> on_new_nt_actions; // действия, осуществляемые при добавлении нового нетерминала в грамматику
    vector<NewTAction> on_new_t_actions;   // действия, осуществляемые при добавлении нового терминала в грамматику

    struct Temp {
        bool used = false;
        struct BElem {
            int i;
            const NTTreeNode* v;
            NTSet M;
        };
        struct PV {
            const NTTreeNode* v;
            int A;
            int B; // reduction: B -> A -> rule
        };
        dvector<vector<BElem>> B;
        dvector<NTSet> F;
        vector<PV> path;
        void clear() { 
            B.clear(); 
            F.clear(); 
            path.clear();
        }
    };
    struct LockTemp {
        GrammarState* g;
        explicit LockTemp(GrammarState* gr) :g(gr) {
            g->temp_used++;
            if (len(g->tmp) < g->temp_used)
                g->tmp.emplace_back(std::make_unique<Temp>());
        }
        Temp* operator->() const { return g->tmp[g->temp_used - 1].get(); }
        ~LockTemp() { g->tmp[--g->temp_used]->clear(); }
        LockTemp() = delete;
        LockTemp(const LockTemp& l) = delete;
        LockTemp(LockTemp&& l) noexcept: g(l.g) { l.g = nullptr; }
        LockTemp& operator=(const LockTemp&) = delete;
        LockTemp& operator=(LockTemp&&) = delete;
    };
    int temp_used = 0;
    CacheVec<Temp> tmp;
    int start = -1;
    //bool finish = false;
    TF tf;
    PEGLexer lex;

    vector<pair<Pos, string>> _err;
    void error(const string &err);

    int addLexerRule(const string& term, const string& re, bool tok=false, bool to_begin = false);
    int addToken(const string& term, const string& re) { return addLexerRule(term, re, true); }
    int addRule(const string &lhs, const vector<string> &rhs, SemanticAction act = SemanticAction(), int id = -1, unsigned lpr = -1, unsigned rpr = -1);
    int addRule(const string &lhs, const vector<string> &rhs, int id, int lpr, int rpr) {
        return addRule(lhs, rhs, SemanticAction(), id, lpr, rpr);
    }
    int ruleId(const string &lhs, const vector<string> &rhs) const;
    bool addRuleAssoc(const string &lhs, const vector<string> &rhs, unsigned pr, int assoc = 0) {
        // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
        return addRule(lhs, rhs, SemanticAction(), -1, pr * 2 + (assoc > 0), pr * 2 + (assoc < 0));
    }
    int addRuleAssoc(const string &lhs, const vector<string> &rhs, int id, unsigned pr, int assoc = 0) {
        // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
        return addRule(lhs, rhs, SemanticAction(), id, pr*2+(assoc>0), pr*2+(assoc<0));
    }
    int addRule(const string &lhs, const vector<string> &rhs, int id) {
        return addRule(lhs, rhs, SemanticAction(), id);
    }
    bool addRule(const CFGRule &r);

    bool addLexerRule(const ParseNode *tokenDef, bool tok, bool to_begin=false);

    GrammarState() {
        ts[""];  // Резервируем нулевой номер терминала, чтобы все терминалы имели ненулевой номер.
        nts[""]; // Резервируем нулевой номер нетерминала, чтобы все нетерминалы имели ненулевой номер.
    }

    ParseNodePtr reduce(ParseNodePtr *pn, const NTTreeNode *v, int nt, int nt1, ParseContext &pt);

    void setStart(const string& start_nt); //< Устанавливает начальный нетерминал грамматики
    int getStartNT(const string& nt);
    void setWsToken(const string& ws) {
        lex.setWsToken(ws);
    }
    /// Добавление специального токена увеличения отступа
    void setIndentToken(const std::string &nm, int enforce = 0) {
        lex.declareIndentToken(nm, ts[nm], enforce);
    }
    void setDedentToken(const std::string &nm, int enforce = 0) {
        lex.declareDedentToken(nm, ts[nm], enforce);
    }
    void setCheckIndentToken(const std::string &nm, int enforce = 0) {
        lex.declareCheckIndentToken(nm, ts[nm], enforce);
    }
    void setEOLToken(const std::string &nm, int enforce = 0) {
        lex.declareEOLToken(nm, ts[nm], enforce);
    }
    void setSOFToken(const std::string &nm, int enforce = 0) {
        lex.declareSOFToken(nm, ts[nm], enforce);
    }
    void setEOFToken(const std::string &nm, int enforce = 0) {
        lex.declareEOFToken(nm, ts[nm], enforce);
    }
    ostream& print_rule(ostream& s, const CFGRule &r)const {
        s << nts[r.A] << " -> ";
        for (auto& rr : r.rhs) {
            if (rr.term)s << ts[rr.num] << " ";
            else s << nts[rr.num] << " ";
        }
        return s;
    }
    void print_rules(ostream &s) const{
        for (auto &r : rules) {
            print_rule(s, r);
            s << "\n";
        }
    }
    std::string sprint_rules()const {
        std::stringstream s;
        print_rules(s);
        return s.str();
    }
    void addNewNTAction(const NewNTAction& action) {
        on_new_nt_actions.push_back(action);
    }
    void addNewTAction(const NewTAction& action) {
        on_new_t_actions.push_back(action);
    }
};

class ParseContext {
    int _qid = -1;
    bool _qq = false;
    std::shared_ptr<GrammarState> g_;
    shared_ptr<ParseErrorHandler> error_handler_;
    GCAlloc<ParseNode> pnalloc_;
    SpecialLexerAction specialQQAction;
public:
    void setSpecialQQAction(SpecialLexerAction a){
        specialQQAction = a;
    }
    void setErrorHandler(ParseErrorHandler* h){
        error_handler_.reset(h);
    }
    ParseErrorHandler& error_handler() const {
        return *error_handler_;
    }
    bool inQuote() const { return _qq; }
    GrammarState& grammar() const { return *g_; }
    const std::shared_ptr<GrammarState>& grammar_ptr()const{ return g_; }
    void setGrammar(const std::shared_ptr<GrammarState>& pg) {
        if(_qq)
            throw Exception("Cannot change grammar while in quasiquote parse");
        g_ = pg;
    }
    void setQuoteRuleId(int id) {
        _qid = id;
    }
    template<class ... Args>
    ParseNodePtr newnode(Args && ... args) {
        return ParseNodePtr(pnalloc_.allocate(args...));
    }
    void del(ParseNodePtr& x) {
        ParseNode* y = x.get();
        x.reset();
        if (y && !y->refs)
            pnalloc_.deallocate(y);
    }
    ParseContext(): g_(new GrammarState), error_handler_(new ParseErrorHandler) {}
    explicit ParseContext(GrammarState *gg): g_(gg) {}
    explicit ParseContext(std::shared_ptr<GrammarState> &gg): g_(gg) {}

    ParseContext(const ParseContext&px) {
        copy_from(px);
    }

    void copy_from(const ParseContext& px) {
        _qid = px._qid;
        _qq = px._qq;
        g_ = make_shared<GrammarState>(*px.g_);
        error_handler_ = px.error_handler_;
        specialQQAction = px.specialQQAction;
    }
    ParseContext& operator=(const ParseContext& px) {
        copy_from(px);
        return *this;
    }

    ParseNode* quasiquote(const string& nt, const string& q, const std::initializer_list<ParseNode*>& args, int qid, int qmanyid);
    ParseNode* quasiquote(const string& nt, const string& q, const std::vector<ParseNode*>& args, int qid, int qmanyid);

    ParseNode *quasiquote(const string &nt, const string &q, const vector<ParseNodePtr> &args, int qid, int qmanyid);
};

struct LAInfo {
    NTSet t, nt;
    LAInfo& operator |=(const LAInfo &i) { 
        t |= i.t;   nt |= i.nt;
        return *this;
    }
};
using LAMap = PosHash<int, LAInfo>;

struct RulePos {
    mutable const NTTreeNode* sh = nullptr;
    const NTTreeNode *v = nullptr; // Текущая вершина в дереве правил
    NTSet M;                 // Множество нетерминалов в корне дерева
};

struct LR0State {
    VectorF<RulePos,4> v;
    LAMap la; // Информация о предпросмотре для свёртки по нетерминалам до данного фрейма: по нетерминалу A возвращает, какие символы могут идти после свёртки по A
};


struct SStack {
    vector<LR0State> s;
};

struct PStack {
    vector<ParseNodePtr> s;
};


class ParserState {
    LR0State s0;
    GrammarState* g = nullptr;
    ParseContext* pt = nullptr;
    LexIterator lit;
    SStack ss;
    PStack sp;
    enum State {
        AtStart,
        Paused,
        AtEnd
    };
    State state = AtStart;
public:
    bool atEnd() const{ return state == AtEnd; }
    ParserState(ParseContext *px, std::string txt, const string &start = "");
    ParseTree parse_next();
};
ParseTree parse(ParseContext &pt, const std::string &text, const string &start = "", const SpecialLexerAction& la = {});

struct ParserError : public Exception {
    Location loc;
    string err;
    ParserError(Location l, string e): Exception(), loc(l), err(move(e)) {}
    [[nodiscard]] const char *what()const noexcept override { return err.c_str(); }
};

void print_tree(std::ostream& os, ParseTree& t, GrammarState* g);
void print_tree(std::ostream& os, ParseNode *pn, GrammarState* g);

string tree2str(ParseTree& t, GrammarState* g);
string tree2str(ParseNode* pn, GrammarState* g);

void tree2file(const string& fn, ParseTree& t, GrammarState* g);

/** Рекурсивно заменяет листья в поддереве n с rule_id=QExpr, на соответствующие поддеревья */
template<class It>
ParseNode* replace_trees_rec(ParseNode* n, It& pos, const It& end, int nargs, int rnum, int qmanyid, bool *many) {
    if (n->rule_id == rnum || n->rule_id == qmanyid) {
        if (pos == end)
            throw ParserError{ n->loc, "not enough arguments for quasiquote, {} given"_fmt(nargs) };
        ParseNode* res = &**pos;
        if(n->rule_id == qmanyid && many) {
            n->rule_id = qmanyid;
            *many = true;
        }
        ++pos;
        return res;
    }
    bool m = false;
    size_t nch = 0;
    for (auto& ch : n->ch) {
        ch = replace_trees_rec(ch, pos, end, nargs, rnum, qmanyid, &m);
        nch += ch->rule_id==qmanyid ? ch->ch.size() : 1;
    }
    if(m) {
        vector<ParseNode*> v(nch, nullptr);
        size_t p = 0;
        for(auto* ch : n->ch) {
            if(ch->rule_id==qmanyid) {
                for (auto *cc : ch->ch)
                    v[p++] = cc;
            } else v[p++] = ch;
        }
        n->ch = std::move(v);
    }
    return n;
}
