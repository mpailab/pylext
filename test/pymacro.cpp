#include "pymacro.h"

#include <iostream>
#include <algorithm>
//#include <filesystem>
//#include <fstream>
//#include <iomanip>
#include "Parser.h"
#include "base.h"
#include "format.h"

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
	return replace_trees_rec(n, pos, nodes.end(), len(nodes), QExpr, QStarExpr, nullptr);
}

/** Раскрывает определение макроса, заданное в виде дерева разбора
 *  Определение макроса преобразуется в определение функции, раскрывающей этот макрос
 *  При этом в текущий контекст добавляется новое правило, реализуемое этим макросом
 *  @param px  -- текущий контекст
 *  @param n -- корень дерева разбора определения макроса
 *  @param off -- номер дочернего узла, соответствующего имени макроса
 *  @param fnm -- имя функции, на которую заменяется макроопределение
 * */
int conv_macro(ParseContext& px, ParseNodePtr& n, int off, const string &fnm,
               bool macro) {
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
		qq+="${} $$DEDENT"_fmt(px.grammar().nts[stmts->nt]);
        stmts = px.quasiquote("suite", qq, { stmts }, QExpr, QStarExpr);
		n->ch[off+2] = stmts; //px.quasiquote("suite", "\n $stmts1\n", { stmts }, QExpr);
	}
	int rule_num = px.grammar().addRule(n[off].term, rhs);
	string funcdef = R"(@{}_rule("{}",[)"_fmt(macro ? "macro" : "syntax", n[off].term);
	for(int i = 0; i<len(rhs); i++){
	    if(i) funcdef+=',';
        ((funcdef += '"') += rhs[i]) += '"';
	}
	funcdef += "])\ndef " + fnm + arglist + ": $func_body_suite";
	n.reset(px.quasiquote("stmt", funcdef, { n->ch[off+2] }, QExpr, QStarExpr));
	return rule_num;
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
void make_qqir(ParseContext& px, ParseNodePtr& root, ParseNode* n, const std::string& nt)
{
    int qqp = len(n->ch) / 2;
    vector<ParseNode *> qargs(qqp); //n->ch.size());

    string qq  = "quasiquote(\"";
    qq += nt;
    qq += "\",[";
    for (int i = 0; i < len(n->ch); i += 2) {
        if (i)
            qq += ',';
        // TODO: Парсить f string и раскрывать макросы в выражениях
        if (n->ch[i]->term.find('{') != string::npos)
            qq += 'f';
        tostr(qq, n->ch[i]->term, '"');
    }

    qq += "],[";
    for (int i = 0; i < qqp; i++) {
        if (i)
            qq += ',';
        qargs[i] = n->ch[2 * i + 1];
        qq += "$expr";
    }
    qq += "])";

    root.reset(px.quasiquote("expr", qq, qargs, QExpr, QStarExpr));
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
    string qq; // = parts[0];
    for(int i=0; i<len(subtrees); i++) {
        qq += parts[i];
        ((qq += '$') += px.grammar().nts[subtrees[i]->nt])+=' ';
    }
    qq += parts.back();
    try {
        return ParseNodePtr(px.quasiquote(nt, qq, subtrees, QExpr, QStarExpr));
    } catch (Exception &e){
        e.prepend_msg("In quasiquote `{}`: "_fmt(qq));
        throw move(e);
    }
}
void check_quote(ParseContext& px, ParseNodePtr& n){
    if(!px.inQuote())
        throw GrammarError("$<ident> outside of quasiquote");
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
    //if(!px->grammar())px->g = new GrammarState;
	//px->g->data = PyMacroModule();
	const shared_ptr<GrammarState>& pg = px->grammar_ptr();
	ParseContext px0;
	GrammarState& g0 = px0.grammar();

    pg->addNewNTAction([](GrammarState* g, const string& ntn, int nt) {
		addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), check_quote, QExpr);
        // addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), check_quote, QExpr);
        addRule(*g, "{} -> '${}_many'"_fmt(ntn, ntn), check_quote, QStarExpr);
        addRule(*g, "qqst -> '{}`' {} '`'"_fmt(ntn, ntn));
	});
    pg->setWsToken("ws");
    pg->addLexerRule("ws", R"(([ \t\n\r] / comment)*)");

    init_base_grammar(g0, pg.get());
    addRule(g0, "rule_symbol -> '{' rule_rhs '}'", [px](ParseContext& pt, ParseNodePtr& n) {
        // auto &px = static_cast<PythonParseContext&>(pt);
        auto v = getVariants(n->ch[0]);
        if(v.size()>1)throw GrammarError("Error in grammar: many cannot be applied to expression containing several variants");
        auto &ntname = px->ntmap[v];
        string manyntname;
        if (ntname.empty()) {
            if(v[0].size()==1&&v[0][0].size()==1) ntname = v[0][0][0];
            else                                  ntname = "__nt_{}"_fmt(px->ntmap.size());
            manyntname = ntname+"_many";
            px->grammar().addRule(ntname, v[0]);
            addRule(px->grammar(), "{}_many -> {}"_fmt(ntname, ntname));
            addRule(px->grammar(), "{}_many -> {}_many {}"_fmt(ntname, ntname, ntname), flatten);
        } else manyntname = ntname+"_many";
        n->ch[0] = px->newnode().get();
        n->ch[0]->term = manyntname;
        n->ch[0]->nt = px->grammar().ts["ident"];
    });

	g0.addLexerRule("comment", "'#' [^\\n]*");
	string text = loadfile("syntax/python.txt");
	parse(px0, text);

    pg->setStart("text");
	// g.setSOFToken("SOF");
	addRule(*pg, "text -> INDENTED root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
	    if(read_by_stmt) {
            n.reset(n->ch[0]);
            n->type = ParseNode::Final;
        }
	});
    addRule(*pg, "text -> text INDENTED root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
        if(read_by_stmt) {
            n.reset(n->ch[1]);
            n->type = ParseNode::Final;
        }
    });

    pg->addToken("qqopen", "ident '`'");
    pg->addToken("qqmid", "('\\\\' [^] / [^`$] / '$$')*");
    pg->addToken("qqmidfirst", "!'`' qqmid");
	addRule(*pg, "qqst -> qqmidfirst"); // qqmid ");
	addRule(*pg, "qqst -> qqst '$' ident qqmid", flatten);
	addRule(*pg, "qqst -> qqst '${' expr '}' qqmid", flatten);
	addRule(*pg, "expr -> '`' qqst '`'", make_qq);
    addRule(*pg, "expr -> '``' qqst '``'", make_qq);
    addRule(*pg, "expr -> '```' qqst '```'", make_qq);
    addRule(*pg, "expr -> ident '`' qqst '`'", make_qqi);
    addRule(*pg, "expr -> ident '``' qqst '``'", make_qqi);
    addRule(*pg, "expr -> ident '```' qqst '```'", make_qqi);
    //setDebug(1);

    addRule(*pg, "syntax_elem -> ident", MacroArgToken);
    addRule(*pg, "syntax_elem -> stringliteral", MacroConstStr);
	addRule(*pg, "syntax_elem -> ident ':' ident", MacroArg);
	addRule(*pg, "syntax_elem -> ident ':' '*' ident", MacroArgExpand);

	addRule(*pg, "syntax_elems -> ',' syntax_elem");
	addRule(*pg, "syntax_elems -> syntax_elems ',' syntax_elem", flatten);
	addRule(*pg, "root_stmt -> 'syntax' '(' ident syntax_elems ')' ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
		string fnm = px.module.uniq_name("syntax_" + n[0].term);
		int id = conv_macro(px, n, 0, fnm, false);
        px.module.syntax[id] = PySyntax{fnm, id};
	});
	addRule(*pg, "root_stmt -> 'defmacro' ident '(' ident syntax_elems ')' ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
		string fnm = px.module.uniq_name("macro_" + n[0].term);
		int id = conv_macro(px, n, 1, fnm, true);
        px.module.macros[id] = PyMacro{ fnm, id };
	});
    px->setSpecialQQAction([](PEGLexer* lex, const char *s, int &pos) -> int {
        while (isspace(s[pos]) && s[pos] != '\n')
            pos++;
        if (s[pos] == '$') {
            if (s[pos + 1] == '$') {
                int q = pos + 2;
                for (; isalnum(s[q]) || s[q] == '_';)
                    q++;
                string id(s + pos + 2, q - pos - 2);
                int num = lex->internalNumCheck(id);
                if (num < 0)
                    throw SyntaxError(
                            "Invalid token {} at {}: `{}` not a token name"_fmt(Substr{s + pos, q - pos}, lex->cpos(),
                                                                                Substr{s + pos + 2, q - pos - 2}));
                pos = q;
                // while(isspace(pos+1))
                return num;
            } else {
                int q = pos + 1;
                for (; isalnum(s[q]) || s[q] == '_';)
                    q++;
                string id(s + pos, q - pos);
                auto* pnum = lex->cterms(id.c_str());
                if (!pnum)
                    throw SyntaxError("Invalid token {} at {}"_fmt(Substr{s + pos, q - pos}, lex->cpos()));
                pos = q;
                // while(isspace(pos+1))
                return *pnum;
            }
        }
        return -1;
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
    //auto *g = new GrammarState;
    auto *px =  new PythonParseContext;
    init_python_grammar(px, by_stmt != 0);
    cout<<"create px = "<<px<<endl;
    return px;
}

void del_python_context(void *px) {
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
    return addRule(((ParseContext*)px)->grammar(), string(lhs)+" -> "+rhs);
}

void* new_parser_state(void *px, const char* text, const char *start) {
    //cout<<"px = "<<px<<endl;
    //cout<<"text = "<<text<<endl;
    //cout<<"start = "<<start<<endl;
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
    auto s = tree2str(node, px->grammar_ptr().get());
    buf.resize(s.size()+1);
    memcpy(buf.data(), s.c_str(), s.size()+1);
    //cout<<buf.data()<<endl;
    return buf.data();
}
