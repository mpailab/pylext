#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
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
//#include "BinAlloc.h"


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
//using PParseNode = unique_ptr<ParseNode>;
struct ParseTree;

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
	//ParseTree* tree = 0;  // Ссылка на дерево разбора
	//ParseNode* p = 0;     // Ссылка на родительский узел
	int size = 1;
	unsigned lpr = -1, rpr = -1; // Левый и правый приоритеты (если unsigned(-1), то приоритеты не заданы)
	vector<ParseNode* /*,BinAlloc<ParseNode>*/> ch; // Дочерние узлы
	Location loc;         // Размещение фрагмента в тексте
	[[nodiscard]] bool isTerminal()const {
		return rule < 0;
	}
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
	ParseNode* balancePr() {		// TODO: обавить проверку, что нет неоднозначности свёртки разных нетерминалов, которые не сравниваются по приоритетам 						                            
		if (haslpr() && nt == ch[0]->nt)					  /*        this                           r        */
			lpr = min(lpr, ch[0]->lpr);						  /*       / .. \ 			              / \ 		*/
		if (hasrpr()) {										  /*      x...   r			             .   .  	*/
			ParseNode *pn = ch.back(), *pp;					  /*            / \ 		            .     .		*/
			if (pn->nt == nt && pn->lpr < rpr) {			  /*           .   .     ==>           .........	*/
				do {										  /*          .     .		          /				*/
					pn->lpr = min(pn->lpr, lpr);			  /*         .........		         pp 			*/
					pp = pn; pn = pn->ch[0];				  /*        /				       / .. \ 			*/
				} while (pn->nt == nt && pn->lpr < rpr);      /*      pp				   this   ...y			*/
				ParseNode *l = pp->ch[0], *r = ch.back();     /*    / .. \ 				  / .. \ 				*/
				pp->ch[0] = this;                             /*   pn  ...y				 x...  pn				*/
				ch.back() = l;  
				return r; // корнем становится r
			}
		}
		return this; // корень остаётся прежним
	}
	ParseNode& operator=(ParseNode&&) = default;
	//void del();
	~ParseNode() = default;
	void serialize(vector<unsigned char>& res);
};

using ParseNodePtr = GCPtr<ParseNode>;
class GrammarState;
struct SStack;
struct PStack;

class ParseErrorHandler {
public:
	struct alignas(8) Hint {
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

class ParseContext {
    int _qid = -1;
    bool _qq = false;
public:
    [[nodiscard]] bool inQuote() const{ return _qq; }
	GrammarState* g=0;
	shared_ptr<ParseErrorHandler> error_handler;
	GCAlloc<ParseNode> pnalloc;
	void setQuoteRuleId(int id) {
		_qid = id;
	}
	template<class ... Args>
	ParseNodePtr newnode(Args && ... args) {
		return ParseNodePtr(pnalloc.allocate(args...));
	}
	void del(ParseNodePtr& x) {
		ParseNode* y = x.get();
		x.reset();
		if (y && !y->refs)
			pnalloc.deallocate(y);
	}
	explicit ParseContext(GrammarState *gg=0):g(gg){}
	ParseNode* quasiquote(const string& nt, const string& q, const std::initializer_list<ParseNode*>& args, int qid=-1);
	ParseNode* quasiquote(const string& nt, const string& q, const std::vector<ParseNode*>& args, int qid=-1);

    ParseNode *quasiquote(const string &nt, const string &q, const vector<ParseNodePtr> &args, int qid);
};

struct ParseTree {
	ParseNodePtr root;
	ParseTree() = default;
	ParseTree(ParseTree && t) noexcept = default;
    explicit ParseTree(ParseNode* pn): root(pn) {}
};

template<class T>
struct Ref : unique_ptr<T> {
	Ref() :unique_ptr<T>(new T) {}
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
		if (it == ntEdges.end())return 0;
		return it->second.get();
	}
	[[nodiscard]] int rule(int A)const {
		return rules.find(A)->second;
	}
};

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
			//T[rhs0].add(A);
			for(int x : inv[rhs0])
				T[x] |= T[A]; // B -> A ..., A -> rhs0 ... ==> B -> rhs0 ...

			//inv[A].add(rhs0);
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
		//int mx = max(A, rhs0);
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
struct alignas(8) RuleElem {
	int num; // Номер
	bool cterm; // для терминала: является ли он константным
	bool term;  // true => терминал, false => нетерминал
	bool save;  // Следует ли данный элемент сохранять в дереве разбора; по умолчанию save = !cterm, поскольку только неконстантные элементы имеет смысл сохранять
};

class GrammarState;
typedef function<void(ParseContext& g, ParseNodePtr&)> SemanticAction;

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
public:
	int mx = -1;
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

class GrammarState {
public:
	any data;
	using NewNTAction = function<void(GrammarState*, const string&, int)>; // Обработчик события добавления нового нетерминала
	using NewTAction  = function<void(GrammarState*, const string&, int)>; // Обработчик события добавления н6ового терминала

	unordered_map<int, NTSet> tFirstMap;   // По терминалу возвращает, какие нетерминалы могут начинаться с данного терминала
	vector<vector<NTTreeNode*>> ntRules;   // Каждому нетерминалу сопоставляется список финальных вершин, соответствующих правилам для данного нетерминала
	NTTreeNode root;        // Корневая вершина дерева правил
	Enumerator<string, unordered_map> nts; // Нумерация нетерминалов
	Enumerator<string, unordered_map> ts;  // Нумерация терминалов
	unordered_map<string, int> _start_nt;
	vector<CFGRule> rules;

	vector<NewNTAction> on_new_nt_actions;
	vector<NewTAction> on_new_t_actions;

	struct Temp {

		bool used = false;
		struct BElem {
			int i;
			const NTTreeNode* v;
			NTSet M;
		};
		struct alignas(16) PV {
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
				g->tmp.emplace_back(new Temp);
		}
		Temp* operator->() const { return g->tmp[g->temp_used - 1].get(); }
		~LockTemp() { g->tmp[--g->temp_used]->clear(); }
		LockTemp() = delete;
		LockTemp(const LockTemp& l) = delete;
		LockTemp(LockTemp&& l) noexcept: g(l.g) { l.g = 0; }
		LockTemp& operator=(const LockTemp&) = delete;
		LockTemp& operator=(LockTemp&&) = delete;
	};
	int temp_used = 0;
	vector<unique_ptr<Temp>> tmp;
	int start = -1;
	bool finish = false;
	TF tf;
	PEGLexer lex;
	//int syntaxDefNT=0;
	//int tokenNT=0;
	vector<pair<Pos, string>> _err;
	void error(const string &err);

	void addLexerRule(const string& term, const string& re, bool tok=false, bool to_begin = false);
	void addToken(const string& term, const string& re) { addLexerRule(term, re, true); }
	int addRule(const string &lhs, const vector<vector<string>> &rhs, SemanticAction act = SemanticAction(), int id = -1, unsigned lpr = -1, unsigned rpr = -1);
	int addRule(const string &lhs, const vector<vector<string>> &rhs, int id, unsigned lpr = -1, unsigned rpr = -1) {
		return addRule(lhs, rhs, SemanticAction(), id, lpr, rpr);
	}
	bool addRuleAssoc(const string &lhs, const vector<vector<string>> &rhs, int id, unsigned pr, int assoc = 0) { // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
		return addRule(lhs, rhs, SemanticAction(), id, pr * 2 + (assoc > 0), pr * 2 + (assoc < 0));
	}
	int addRule(const string &lhs, const vector<string> &rhs, SemanticAction act = SemanticAction(), int id = -1, unsigned lpr = -1, unsigned rpr = -1) {
		vector<vector<string>> vrhs(rhs.size());
		for (int i = 0; i < (int)rhs.size(); i++)
			vrhs[i] = { rhs[i] };
		return addRule(lhs, vrhs, std::move(act), id, lpr, rpr);
	}
	int addRule(const string &lhs, const vector<string> &rhs, int id, unsigned lpr, unsigned rpr) {
		return addRule(lhs, rhs, SemanticAction(), id, lpr, rpr);
	}
	int addRuleAssoc(const string &lhs, const vector<string> &rhs, int id, unsigned pr, int assoc = 0) { // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
		return addRule(lhs, rhs, SemanticAction(), id, pr*2+(assoc>0), pr*2+(assoc<0));
	}
	int addRule(const string &lhs, const vector<string> &rhs, int id) {
		return addRule(lhs, rhs, SemanticAction(), id);
	}
	bool addRule(const CFGRule &r);

	bool addLexerRule(const ParseNode *tokenDef, bool tok, bool to_begin=false);
	//bool addRule(const ParseNode *ruleDef);
	GrammarState() {
		ts[""];  // Резервируем нулевой номер терминала, чтобы все терминалы имели ненулевой номер.
		nts[""]; // Резервируем нулевой номер нетерминала, чтобы все нетерминалы имели ненулевой номер.
	}

	ParseNodePtr reduce(ParseNodePtr *pn, const NTTreeNode *v, int nt, int nt1, ParseContext &pt);

	void setStart(const string& start_nt){ //, const string& token, const string &syntax) {
		int S0 = nts[""];
		this->start = nts[start_nt];
		CFGRule r;
		r.A = S0;
		r.rhs.resize(2);
		r.rhs[0].num = this->start;
		r.rhs[0].save = true;
		r.rhs[0].term = false;
		r.rhs[1].term = true;
		r.rhs[1].save = false;
		r.rhs[1].num = 0;
		addRule(r);
		_start_nt[start_nt] = S0;
		//if(!token.empty())
		//	tokenNT = nts[token];
		//if (!syntax.empty())
		//	syntaxDefNT = nts[syntax];
	}
	int getStartNT(const string& nt) {
	    if (_start_nt.empty()){
	        setStart(nt);
	        _start_nt[nt] = start;
	    }
	    if(_start_nt.count(nt))return _start_nt[nt];
        int S0 = nts["_start_"+nt];
        CFGRule r;
        r.A = S0;
        r.rhs.resize(2);
        r.rhs[0].num = nts[nt];
        r.rhs[0].save = true;
        r.rhs[0].term = false;
        r.rhs[1].term = true;
        r.rhs[1].save = false;
        r.rhs[1].num = 0;
        r.action = [](ParseContext&px, ParseNodePtr & n){ n.reset(n->ch[0]); };
        addRule(r);
        _start_nt[nt] = S0;
        return S0;
	}
	void setWsToken(const string& ws) {
		lex.setWsToken(ws);
	}
	void setIndentToken(const std::string &nm) {
		lex.declareIndentToken(nm, ts[nm]);
	}
	void setDedentToken(const std::string &nm) {
		lex.declareDedentToken(nm, ts[nm]);
	}
	void setCheckIndentToken(const std::string &nm) {
		lex.declareCheckIndentToken(nm, ts[nm]);
	}
	void setEOLToken(const std::string &nm) {
		lex.declareEOLToken(nm, ts[nm]);
	}
    void setSOFToken(const std::string &nm) {
        lex.declareSOFToken(nm, ts[nm]);
    }
	void setEOFToken(const std::string &nm) {
		lex.declareEOFToken(nm, ts[nm]);
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
	void addNewNTAction(const NewNTAction& action) {
		on_new_nt_actions.push_back(action);
	}
	void addNewTAction(const NewTAction& action) {
		on_new_t_actions.push_back(action);
	}
};

struct LAInfo {
	NTSet t, nt;
	LAInfo& operator |=(const LAInfo &i) { 
		t |= i.t;   nt |= i.nt;
		return *this;
	}
};
typedef PosHash<int, LAInfo> LAMap;

struct RulePos {
	mutable const NTTreeNode* sh = 0;
	const NTTreeNode *v = 0; // Текущая вершина в дереве правил
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
    GrammarState* g = 0;
    ParseContext* pt = 0;
    LexIterator lit;
    SStack ss;
    PStack sp;
    // string text;
    enum State {
        AtStart,
        Paused,
        AtEnd
    };
    State state = AtStart;
public:
    ParserState(ParseContext *px, std::string txt, const string &start = "");
    ParseTree parse_next();
};
ParseTree parse(ParseContext &pt, const std::string &text, const string &start = "");

struct ParserError : public Exception {
	Location loc;
	string err;
	ParserError(Location l, string e): Exception(), loc(l), err(move(e)) {}
	[[nodiscard]] const char *what()const noexcept override{return err.c_str();}
};

struct ParseBreak {
    ParseNode* result;
    ParseBreak(ParseNode* res): result(res){}
};

void print_tree(std::ostream& os, ParseTree& t, GrammarState* g);
void print_tree(std::ostream& os, ParseNode *pn, GrammarState* g);

string tree2str(ParseTree& t, GrammarState* g);
string tree2str(ParseNode* pn, GrammarState* g);

void tree2file(const string& fn, ParseTree& t, GrammarState* g);

/** Рекурсивно заменяет листья в поддереве n с rule_id=QExpr, на соответствующие поддеревья */
template<class It>
ParseNode* replace_trees_rec(ParseNode* n, It& pos, const It& end, int rnum) {
	if (n->rule_id == rnum) {
		if (pos == end)
			throw ParserError{ n->loc, "not enough arguments for quasiquote" };
		ParseNode* res = &**pos;
		++pos;
		return res;
	}
	for (auto& ch : n->ch)
		ch = replace_trees_rec(ch, pos, end, rnum);
	return n;
}
