#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <vector>
#include <cctype>
#include <algorithm>
#include <memory>
#include <string>
#include "Exception.h"
#include "PEGLexer.h"
#include "NTSet.h"
//#include "BinAlloc.h"

using namespace std;

struct ParseNode {
	int nt;               // Номер терминала или нетерминала
	int B = -1;           // Номер промежуточного нетерминала в случае свёртки nt <- B <- rule
	int rule=-1;          // Номер правила (-1 => терминал)
	string term;          // Строковое значение (если терминал)
	ParseNode* p = 0;     // Ссылка на родительский узел
	vector<ParseNode/*,BinAlloc<ParseNode>*/> ch; // Дочерние узлы
	Location loc;         // Размещение фрагмента в тексте
	bool isTerminal()const {
		return rule < 0;
	}
	ParseNode& operator[](size_t i) { return ch[int(i)<0 ? i+ch.size() : i]; }
	const ParseNode& operator[](size_t i)const { return ch[int(i)<0 ? i + ch.size() : i]; }
	ParseNode() = default;
	ParseNode(const ParseNode&) = delete;
	ParseNode& operator=(const ParseNode&) = delete;

	ParseNode(ParseNode&&) = default;
	ParseNode& operator=(ParseNode&&) = default;
	void del();
	~ParseNode() {
		if (ch.size())del();
	}
};

//typedef NTSetCmp<NTSetV4,NTSetV> NTSet;
//typedef NTSetV4 NTSet;
typedef NTSetV NTSet;

//struct NTTreeNode;
template<class Node>
struct NTTreeEdge {
	Node end;  // Конец ребра
	//NTSet filter;    // Фильтр на ребре
};
template<class T>
struct Ref : unique_ptr<T> {
	Ref() :unique_ptr<T>(new T) {}
};
struct NTTreeNode {
	//typedef NTTreeEdge<NTTreeNode> Edge;
	//unordered_map<string, Edge> charedges; // Ветвление по константным терминалам
	unordered_map<int, Ref<NTTreeNode>> termEdges;    // Ветвление по неконстантным терминалам
	unordered_map<int, Ref<NTTreeNode>> ntEdges;      // Ветвление по нетерминалам
	NTSet phi;                                   // Нетерминалы, по которым можно пройти до текущей вершины
	NTSet finalNT;                               // Нетерминалы, для которых текущая вершина -- финальная
	NTSet next;                                // Множество нетерминалов, по которым можно пройти дальше по какому-либо ребру
	NTSet nextnt;                              // Множество нетерминалов, по которым можно пройти дальше по нетерминальному ребру
	unordered_map<int, int> rules;             // Правила по номерам нетерминалов
	int pos = 0;                               // Расстояние до корня дерева
	int _frule = -1;
	const NTTreeNode* nextN(int A)const {
		auto it = ntEdges.find(A);
		if (it == ntEdges.end())return 0;
		return it->second.get();
	}
	int rule(int A)const {
		return rules.find(A)->second;
	}
};

struct TF {
	vector<NTSet> T;     // По нетерминалу возвращает все нетерминалы, которые наследуются от данного нетерминала (A :-> {B | B => A})
	vector<NTSet> inv;   // По нетерминалу возвращает все нетерминалы, от которых наследуется данный нетерминал   (A :-> {B | A => B})
	vector<NTSet> fst;   // По нетерминалу возвращает все нетерминалы, с которых может начинаться данный нетерминал
	vector<NTSet> ifst;  // По нетерминалу возвращает все нетерминалы, которые могут начинаться с данного нетерминала
	void addRuleBeg(int /*pos*/, int A, int rhs0, int len) { // Добавляет в структуру правило A -> rhs0 ...; pos -- позиция в тексте; len -- длина правой части правила
		int mx = max(A, rhs0);
		checkSize(mx);
		//if (T.size() <= mx)T.resize(mx + 1), inv.resize(mx + 1);
		if (len == 1) {
			//T[rhs0].add(A);
			for(int x : inv[rhs0])
				T[x] |= T[A]; // B -> A ..., A -> rhs0 ... ==> B -> rhs0 ...

			//inv[A].add(rhs0);
			for(int x : T[A])
				inv[x] |= inv[rhs0]; // A -> rhs0 ..., rhs0 -> B ... ==> A -> B ...
		}
		for(int x : ifst[A])
			fst[x] |= fst[rhs0];
		for(int x : fst[rhs0])
			ifst[x] |= ifst[A];
		// TODO: как-то запоминать позицию и изменения, чтобы можно было потом откатить назад.
	}
	void checkSize(int A) {
		int n0 = (int)T.size();
		if (n0 <= A) {
			T.resize(A + 1), inv.resize(A + 1), fst.resize(A + 1), ifst.resize(A + 1);
			for (int i = n0; i <= A; i++) {
				T[i].add(i);
				inv[i].add(i);
				fst[i].add(i);
				ifst[i].add(i);
			}
		}
	}
};


struct NTTree {
	unordered_map<int, vector<int>> ntFirstMap; // По входному нетерминалу возвращает список всех нетерминалов, с которых данный нетерминал может начинаться
	unordered_map<int, vector<int>> tFirstMap;  // По входному нетерминалу возвращает список всех терминалов, с которых данный нетерминал может начинаться
	NTTreeNode root;
	int start;
};

struct RuleElem {
	int num;
	bool term;
	bool save;
};
struct GrammarState;
typedef function<void(GrammarState *g, ParseNode&)> SemanticAction;

struct CFGRule {
	int A;
	vector<RuleElem> rhs; 
	int used;
	SemanticAction action;
};
/*
template<class Cont>
concept has_clear = requires(Cont x) {
	{x.clear()}->void;
};
template<class T> requires has_clear<T>
void clearElem(T& vec) {
	vec.clear();
}*/

template<class T>
struct dvector {
	vector<T> v;
	int mx = -1;
	const T& operator[](size_t i)const {
		return v[i];
	}
	T& operator[](size_t i) {
		mx = max(mx, (int)i);
		if (i >= v.size())v.resize(i + 1);
		return v[i];
	}
	int size()const { return (int)v.size(); }
	void clear() {
		for (int i = 0; i <= mx; i++)v[i].clear();
		mx = -1;
	}
};

void setDebug(bool b);
struct GrammarState {
	//unordered_map<int, vector<int>> ntFirstMap; // По входному нетерминалу возвращает список всех нетерминалов, с которых данный нетерминал может начинаться
	//unordered_map<int, vector<int>> tFirstMap;  // По входному нетерминалу возвращает список всех терминалов, с которых данный нетерминал может начинаться
	unordered_map<int, NTSet> tFirstMap;   // По терминалу возвращает, какие нетерминалы могут начинаться с данного терминала
	//vector<NTSet> ntFirstMap;              // По нетерминалу возвращает множество нетерминалов, с которых данный нетерминал может начинаться
	vector<vector<NTTreeNode*>> ntRules;   // Каждому нетерминалу сопоставляется список финальных вершин, соответствующих правилам для данного нетерминала
	NTTreeNode root;        // Корневая вершина дерева правил
	Enumerator<string, unordered_map> nts; // Нумерация нетерминалов
	Enumerator<string, unordered_map> ts;  // Нумерация терминалов
	vector<CFGRule> rules;
	struct Temp {
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
	Temp tmp;
	int start;
	bool finish = false;
	TF tf;
	PEGLexer lex;
	int syntaxDefNT=0;
	int tokenNT=0;
	vector<pair<Pos, string>> _err;
	void error(const string &err);

	void addLexerRule(const string& term, const string& re, bool tok=false, bool to_begin = false);
	void addToken(const string& term, const string& re) { addLexerRule(term, re, true); }
	bool addRule(const string &lhs, const vector<string> &rhs, SemanticAction act = SemanticAction());
	bool addRule(const CFGRule &r);

	bool addLexerRule(const ParseNode *tokenDef, bool tok, bool to_begin=false);
	bool addRule(const ParseNode *ruleDef);
	GrammarState() {
		ts[""];  // Резервируем нулевой номер терминала, чтобы все терминалы имели ненулевой номер.
		nts[""]; // Резервируем нулевой номер нетерминала, чтобы все нетерминалы имели ненулевой номер.
	}
	ParseNode reduce(ParseNode *pn, const NTTreeNode *v, int nt, int nt1) {
		int r = v->rule(nt), sz = len(rules[r].rhs);
		ParseNode res;
		res.rule = r;
		res.B = nt;
		res.nt = nt1;
		int szp = 0;
		for (int i = 0; i < sz; i++)
			szp += rules[r].rhs[i].save;
		res.ch.resize(szp);
		for (int i = 0,j=0; i < sz; i++)
			if (rules[r].rhs[i].save)
				res.ch[j++] = move(pn[i]);
		res.loc.beg = pn[0].loc.beg;
		res.loc.end = pn[sz - 1].loc.end;
		if (rules[r].action)
			rules[r].action(this, res);
		//if (nt == tokenNT)addToken(&res);
		//else if (nt == syntaxDefNT)addRule(&res);
		return std::move(res);
	}
	void setNtNames(const string&start, const string& token, const string &syntax) {
		int S0 = nts[""];
		this->start = nts[start];
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
		if(!token.empty())
			tokenNT = nts[token];
		if (!syntax.empty())
			syntaxDefNT = nts[syntax];
	}
	void setWsToken(const string& ws) {
		lex.setWsToken(ws);
	}
	void print_rules(ostream &s) const{
		for (auto &r : rules) {
			s << nts[r.A] << " -> ";
			for (auto& rr : r.rhs) {
				if (rr.term)s << ts[rr.num] << " ";
				else s << nts[rr.num] << " ";
			}
			s << "\n";
		}
	}
};


typedef unordered_map<int, TrieM<int>> FollowR_T;

struct RulePos {
	mutable const NTTreeNode* sh = 0;
	const NTTreeNode *v = 0; // Текущая вершина в дереве правил
	NTSet M;                 // Множество нетерминалов в корне дерева
};

struct LR0State {
	vector<RulePos> v;
};


struct SStack {
	vector<LR0State> s;
};

struct PStack {
	vector<ParseNode> s;
};


ParseNode parse(GrammarState & g, const std::string& text);

struct StackFrame {
	const NTTreeNode *v = 0; // Текущая вершина в дереве правил
	NTSetS nts;         // Текущее множество нетерминалов
	NTSetS leftrec;     // Нетерминалы, для которых из корня процедура вызывается рекурсивно (те, рёбра которых есть исходящие рёбра из корня)
	FollowR_T fr;      // Множество возможных продолжений после свёртки в зависимости от нетерминала
	vector<ParseNode> parsed; // Список разобранных поддеревьев, соответствующих разобранным нетерминалам текущего правила
};

typedef vector<StackFrame> ParseStack;

struct ParseState {
	ParseStack st;
	FollowR_T followR;
	StackFrame&operator[](size_t x) { return st[x]; }
	StackFrame& top() { return st.back(); }
};

struct ParserError {
	Location loc;
	string err;
};
