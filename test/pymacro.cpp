#include <iostream>
//#include <filesystem>
//#include <fstream>
//#include <iomanip>
#include "Parser.h"
#include "base.h"
#include "pymacro.h"
#include "format.h"
#include <algorithm>

using namespace std;

enum MacroRule {
	QExpr = SynTypeLast + 1,
	QStarExpr,
	MacroArg,
	MacroArgExpand,
	MacroConstStr,
	MacroArgToken
};

/** Заменяет листья с rule_id=QExpr, на соответствующие поддеревья */
ParseNode* replace_trees(ParseNode* n, const vector<ParseNode*>& nodes) {
	auto pos = nodes.begin();
	return replace_trees_rec(n, pos, nodes.end(), QExpr, QStarExpr, 0);
}

/** Раскрывает определение макроса, заданное в виде дерева разбора
 *  Определение макроса преобразуется в определение функции, раскрывающей этот макрос
 *  При этом в текущий контекст добавляется новое правило, реализуемое этим макросом
 *  @param px  -- текущий контекст
 *  @param n -- корень дерева разбора определения макроса
 *  @param off -- номер дочернего узла, соответствующего имени макроса
 *  @param fnm -- имя функции, на которую заменяется макроопределение
 * */
int conv_macro(ParseContext& px, ParseNodePtr& n, int off, const string &fnm) {
	vector<string> rhs, expand;
	string arglist = "(";
	for (int i = 0; i < (int)n[off+1].ch.size(); i++) {
		ParseNode& ni = n[off+1][i];
		if (ni.rule_id == MacroArg || ni.rule_id == MacroArgExpand) {
			if (ni.rule_id == MacroArgExpand)
				expand.push_back(ni[0].term);
			rhs.push_back(ni[1].term);
			arglist += ni[0].term;
			arglist += ',';
        } else if (ni.isTerminal()) {
            rhs.push_back(ni.term);
        } else if (ni.ch.size() != 1 || !ni.ch[0]->isTerminal()) {
		    throw GrammarError("Internal error: wrong macro argument syntax tree");
		} else {
            rhs.push_back(ni[0].term);
        }
	}
	if (arglist.size() > 1)arglist.back() = ')';
	else arglist += ')';
	//n[0].term = m->uniq_name("syntax_"+n[0].term);
	if (!expand.empty()) {
		ParseNode* stmts = n[off+2].ch[0];
		string qq = "$$INDENT\n";
		for (auto& arg : expand) {
		    qq+="{}=syn_expand({})\n"_fmt(arg, arg);
		}
		qq+="$*{} $$DEDENT"_fmt(px.g->nts[stmts->nt]);
        stmts = px.quasiquote("suite", qq, { stmts }, QExpr);
		n->ch[off+2] = stmts; //px.quasiquote("suite", "\n $stmts1\n", { stmts }, QExpr);
	}
	n.reset(px.quasiquote("stmt", "def " + fnm + arglist + ": $func_body_suite", { n->ch[off+2] }, QExpr));
	return px.g->addRule(n[off].term, rhs);
}


string& tostr(string &res, const string& str, char c) {
    res += c;
    for (char x : str) {
        if (x == c)(res += '\\') += x;
        else if (x == '\n')res += "\\n";
        else res += x;
    }
    return res += c;
}

string tostr(const string& str, char c) {
	string res;
	return tostr(res, str, c);
}

/**
 * Раскрывает квазицитату, преобразуя её в вызов функции, подставляющей деревья разбора в выражение
 * Квазицитата с N аргументами представляется узлом с 2N+1 дочерними узлами:
 * nt`s0 $arg1 s1 $arg2 s2 ... $argN sN` ~ [s0,arg1,s1,...,arnN,sN]
 * Преобразуется в вызов функции quasiquote("nt", [f"s0",f"s1",...,f"sN"],[arg1,...,argN])
 * Таким образом, в квазицитате можно будет использовать выражения, как в f string
 */
void make_qqir(ParseContext& px, ParseNodePtr& root, ParseNode* n, const std::string& nt) {
    int qqp = len(n->ch)/2;
    vector<ParseNode*> qargs(qqp); //n->ch.size());
    string qq = "quasiquote(\"";
    qq += nt;
    qq += "\",[";
    for (int i = 0; i < len(n->ch); i+=2) {
        if(i) qq += ',';
        // TODO: Парсить f string и раскрывать макросы в выражениях
        if(n->ch[i]->term.find('{')<n[i].term.size()) qq+='f';
        tostr(qq, n->ch[i]->term, '"');
    }

    qq += "],[";
    for (int i = 0; i < qqp; i++) {
        if(i)qq+=',';
        qargs[i] = n->ch[2 * i + 1];
        qq += "$expr";
    }
    qq += "])";

    root.reset(px.quasiquote("expr", qq, qargs, QExpr));
}


void make_qqi(ParseContext& px, ParseNodePtr& n) {
    make_qqir(px, n, n->ch[1], n->ch[0]->term);
}


void make_qq(ParseContext& px, ParseNodePtr& n) {
    make_qqir(px, n, n->ch[0], "expr");
}

ParseNodePtr quasiquote(ParseContext& px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees){
    if (parts.size() != subtrees.size()+1)
        throw GrammarError("in quasiquote nubmer of string parts = {}, number of subtrees = {}"_fmt(parts.size(), subtrees.size()));
    string qq = parts[0];
    for(int i=0; i<len(subtrees); i++) {
        qq += parts[i];
        (qq += '$') += px.g->nts[subtrees[i]->nt];
    }
    qq += parts.back();
    try {
        return ParseNodePtr(px.quasiquote(nt, qq, subtrees));
    } catch (Exception &e){
        e.prepend_msg("In quasiquote `{}`: "_fmt(qq));
        throw e;
    }
}

/**
* Инициализируется начальное состояние системы макрорасширений питона:
* Оно представляет собой объект класса PyMacroModule, который содержит следующую информацию:
* - Грамматика питона + базовые расширения, позволяющие писать макросы:
*    - квазицитаты `... ${..} ... $id ... ... `
*    - конструкция syntax(Rule): <definition>
*    - конструкция defmacro <name>(Rule): <definition>
* - Вспомогательная грамматика для квазицитат. Она включает 
*   все правила исходной грамматики + следующие дополнительные правила:
*    - N -> '$N' для каждого нетерминала N 
*    - T -> '$$T' для каждого неконстантного терминала T (добавляется в лексер)
*/
void init_python_grammar(PythonParseContext* px, bool read_by_stmt) {
    setDebug(0);
    if(!px->g)px->g = new GrammarState;
	//px->g->data = PyMacroModule();
	shared_ptr<GrammarState> pg(px->g, [](GrammarState*) {});
	GrammarState g0;
    px->g->addNewNTAction([](GrammarState* g, const string& ntn, int nt) {
		addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), [](ParseContext& px, ParseNodePtr& n){
		    if(!px.inQuote())
		        throw GrammarError(n->ch[0]->term + " outside of quasiquote");
		}, QExpr);
        addRule(*g, "{} -> '$*{}_many'"_fmt(ntn, ntn), [](ParseContext& px, ParseNodePtr& n){
            if(!px.inQuote())
                throw GrammarError(n->ch[0]->term + " outside of quasiquote");
        }, QStarExpr);
        addRule(*g, "qqst -> '{}`' {} '`'"_fmt(ntn, ntn));
	});
    px->g->setWsToken("ws");
    px->g->addLexerRule("ws", R"(([ \t\n\r] / comment)*)");

    init_base_grammar(g0, px->g);
	g0.addLexerRule("comment", "'#' [^\\n]*");
	string text = loadfile("syntax/python.txt");
	ParseContext px0(&g0);
	parse(px0, text);

    px->g->setStart("text");
	// g.setSOFToken("SOF");
	addRule(*px->g, "text -> INDENTED root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
	    if(read_by_stmt) {
            n.reset(n->ch[0]);
            n->type = ParseNode::Final;
        }
	});
    addRule(*px->g, "text -> text INDENTED root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
        if(read_by_stmt) {
            n.reset(n->ch[1]);
            n->type = ParseNode::Final;
        }
    });

    px->g->addToken("qqopen", "ident '`'");
    px->g->addToken("qqmid", "('\\\\' [^] / [^`$] / '$$')*");
    px->g->addToken("qqmidfirst", "!'`' qqmid");
	//g.addToken("qqmid00", "('\\\\' [^] / [^`$])* '$' &ident");
	//g.addToken("qqmid01", "('\\\\' [^] / [^`$])* '${'");
	//g.addToken("qqbeg1", "`qqpr1");
	//g.addToken("qqbeg0", "`qqpr0");
	//g.addToken("qqmid11", "'}' qqmid01");
	//g.addToken("qqmid10", "'}' qqmid00");
	addRule(*px->g, "qqst -> qqmidfirst"); // qqmid ");
	addRule(*px->g, "qqst -> qqst '$' ident qqmid", flatten);
	addRule(*px->g, "qqst -> qqst '${' expr '}' qqmid", flatten);
	addRule(*px->g, "expr -> '`' qqst '`'", make_qq);
    addRule(*px->g, "expr -> '``' qqst '``'", make_qq);
    addRule(*px->g, "expr -> '```' qqst '```'", make_qq);
    addRule(*px->g, "expr -> ident '`' qqst '`'", make_qqi);
    addRule(*px->g, "expr -> ident '``' qqst '``'", make_qqi);
    addRule(*px->g, "expr -> ident '```' qqst '```'", make_qqi);
    setDebug(1);

    //g.addToken("sq_string", (R"('\'' ('\\' [^] / [^\n'])* '\'')"));
    addRule(*px->g, "syntax_elem -> ident", MacroArgToken);
    addRule(*px->g, "syntax_elem -> stringliteral", MacroConstStr);
	addRule(*px->g, "syntax_elem -> ident ':' ident", MacroArg);
	addRule(*px->g, "syntax_elem -> ident ':' '*' ident", MacroArgExpand);

	addRule(*px->g, "syntax_elems -> ',' syntax_elem");
	addRule(*px->g, "syntax_elems -> syntax_elems ',' syntax_elem", flatten);
	addRule(*px->g, "root_stmt -> 'syntax' '(' ident syntax_elems ')' ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
		string fnm = px.module.uniq_name("syntax_" + n[0].term);
		int id = conv_macro(px, n, 0, fnm);
        px.module.syntax[id] = PySyntax{fnm, id};
	});
	addRule(*px->g, "root_stmt -> 'defmacro' ident '(' ident syntax_elems ')' ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
		string fnm = px.module.uniq_name("macro_" + n[0].term);
		int id = conv_macro(px, n, 0, fnm);
        px.module.macros[id] = PyMacro{ fnm, id };
	});
    addRule(*px->g, "rule_symbol -> '{' rule_rhs '}'", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
        auto v = getVariants(n->ch[1]);
        if(v.size()>1)throw GrammarError("Error in grammar: many cannot be applied to expression containing several variants");
        auto &ntname = px.ntmap[v];
        string manyntname;
        if (ntname.empty()) {
            if(v[0].size()==1&&v[0][0].size()==1) ntname = v[0][0][0];
            else                                  ntname = "__nt_{}"_fmt(px.ntmap.size());
            manyntname = ntname+"_many";
            px.g->addRule(ntname, v[0]);
            addRule(*px.g, "{}_many -> {}"_fmt(manyntname, ntname));
            addRule(*px.g, "{}_many -> {}_many {}"_fmt(manyntname, manyntname, ntname), flatten);
        } else manyntname = ntname+"_many";
        n->ch[0] = px.newnode().get();
        n->ch[0]->term = manyntname;
        n->ch[0]->nt = px.g->ts["ident"];
    });
}

bool equal_subtrees(ParseNode* x, ParseNode* y) {
    if(x->isTerminal())
        return y->isTerminal() && x->term==y->term;
    if(x->ch.size()!=y->ch.size())return false;
    for(int i=0; i<len(x->ch); i++)
        if(!equal_subtrees(x->ch[i], y->ch[i]))
            return false;
    return true;
}

void* new_python_context(int by_stmt) {
    auto *g = new GrammarState;
    auto *px =  new PythonParseContext(g);
    init_python_grammar(px, by_stmt != 0);
    cout<<"create px = "<<px<<endl;
    return px;
}

void del_python_context(void *px) {
    if(px) delete ((ParseContext*)px)->g;
    delete (ParseContext*)px;
}

void* c_quasiquote(void* px, char* nt, int n, char** data, void** pn){
    vector<string> qp(data, data+n);
    vector<ParseNode*> subtrees((ParseNode**)pn, (ParseNode**)pn+n-1);
    ParseNodePtr res = quasiquote(*(ParseContext*)px, nt, qp, subtrees);
    return res.get();
}

void inc_pn_num_refs(void *pn) {
    if(pn) ((ParseNode*)pn)->refs++;
}

void dec_pn_num_refs(void *pn) {
    if(pn) ((ParseNode*)pn)->refs--;
}

int pn_equal(void *pn1, void *pn2) {
    return equal_subtrees((ParseNode*)pn1, (ParseNode*)pn2);
}

int get_pn_num_children(void* pn) {
    return len(((ParseNode*)pn)->ch);
}

void* get_pn_child(void* pn, int i) {
    if(i<0 || i>=len(((ParseNode*)pn)->ch))
        throw Exception("Parse node child index {} out of range ({})"_fmt(i, len(((ParseNode*)pn)->ch)));
    return ((ParseNode*)pn)->ch[i];
}

void set_pn_child(void* pn, int i, void* ch) {
    if(!ch)throw Exception("Cannot set null parse node as child");
    if(i<0 || i>=len(((ParseNode*)pn)->ch))
        throw Exception("Parse node child index {} out of range ({})"_fmt(i, len(((ParseNode*)pn)->ch)));
    ((ParseNode*)pn)->ch[i] = (ParseNode*)ch;
}

int get_pn_rule(void* pn) {
    return ((ParseNode*)pn)->rule;
}

int add_rule(void* px, char* lhs, char *rhs) {
    return addRule(*((ParseContext*)px)->g, string(lhs)+" -> "+rhs);
}

void* new_parser_state(void *px, const char* text, const char *start) {
    cout<<"px = "<<px<<endl;
    cout<<"text = "<<text<<endl;
    cout<<"start = "<<start<<endl;
    return new ParserState((ParseContext*)px, text, start);
}

void* continue_parse(void *state) {
    ParseTree tree = ((ParserState*)state)->parse_next();
    return tree.root.get();
}

void del_parser_state(void* state) {
    delete (ParserState*)state;
}

char* ast_to_text(void* pcontext, void *pn) {
    auto *node = (ParseNode*)pn;
    auto *px = (ParseContext*)pcontext;
    static vector<char> buf;
    auto s = tree2str(node, px->g);
    buf.resize(s.size()+1);
    memcpy(buf.data(), s.c_str(), s.size()+1);
    return buf.data();
}
