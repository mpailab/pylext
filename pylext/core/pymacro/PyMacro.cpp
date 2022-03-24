#include "PyMacro.h"

#include <iostream>
#include <algorithm>
#include "Parser.h"
#include "GrammarUtils.h"
#include "format.h"

using namespace std;

enum MacroRule {
	QExpr = SynTypeLast + 1,
	QStarExpr,
	MacroArg,
	MacroArgExpand,
	MacroConstStr,
	MacroArgToken,
    QQSingle,
    QQMany,
};

/** Заменяет листья с rule_id=QExpr, на соответствующие поддеревья */
ParseNode* replace_trees(ParseNode* n, const vector<ParseNode*>& nodes) {
	auto pos = nodes.begin();
	return replace_trees_rec(n, pos, nodes.end(), len(nodes), QExpr, QStarExpr, nullptr);
}

int add_macro_rule(ParseContext& px, ParseNodePtr& n, int off) {
    vector<string> rhs, expand;
    for (int i = 0; i < (int)n[off+1].ch.size(); i++) {
        ParseNode& ni = n[off+1][i];
        if (ni.rule_id == MacroArg || ni.rule_id == MacroArgExpand) {
            if (ni.rule_id == MacroArgExpand)
                expand.push_back(ni[0].term);
            rhs.push_back(ni[1].term);
        } else if (ni.isTerminal()) {
            rhs.push_back(ni.term);
        } else if (ni.ch.size() != 1 || !ni.ch[0]->isTerminal()) {
            throw GrammarError("Internal error: wrong macro argument syntax tree");
        } else {
            rhs.push_back(ni[0].term);
        }
    }
    return px.grammar().addRule(n[off].term, rhs);
}

/** Раскрывает определение макроса, заданное в виде дерева разбора
 *  Определение макроса преобразуется в определение функции, раскрывающей этот макрос
 *  При этом в текущий контекст добавляется новое правило, реализуемое этим макросом
 *  @param px  -- текущий контекст
 *  @param n -- корень дерева разбора определения макроса
 *  @param off -- номер дочернего узла, соответствующего имени макроса
 *  @param fnm -- имя функции, на которую заменяется макроопределение
 * */
int conv_macro(ParseContext& px, ParseNodePtr& n, int off, const string &fnm, string decorator_name) {
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

	if (!expand.empty()) {
		ParseNode* stmts = n[off+2].ch[0];
		string qq = "\n$$INDENT\n";
		for (auto& arg : expand) {
		    qq+="{}=syn_expand({})\n"_fmt(arg, arg);
		}
		qq+="*${} $$DEDENT"_fmt(px.grammar().nts[stmts->nt]);
        stmts = px.quasiquote("suite", qq, { stmts }, QExpr, QStarExpr);
		n->ch[off+2] = stmts;
	}
	int rule_num = px.grammar().addRule(n[off].term, rhs);
	string funcdef = R"(@{}("{}",[)"_fmt(decorator_name, n[off].term);
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

/**
 * Раскрывает квазицитату, преобразуя её в вызов функции, подставляющей деревья разбора в выражение
 * Квазицитата с N аргументами представляется узлом с 2N+1 дочерними узлами:
 * nt`s0 $arg1 s1 $arg2 s2 ... $argN sN` ~ [s0,arg1,s1,...,arnN,sN]
 * Преобразуется в вызов функции quasiquote("nt", [f"s0",f"s1",...,f"sN"],[arg1,...,argN])
 * Таким образом, в квазицитате можно будет использовать выражения, как в f string
 */
void make_qqir(ParseContext& px, ParseNodePtr& root, ParseNode* n, const std::string& nt, const char*pref=nullptr, const char* suf=nullptr)
{
    int qqp = len(n->ch) / 2;
    vector<ParseNode *> qargs(qqp);

    string qq  = "quasiquote(\"";
    qq += nt;
    qq += "\",[";
    for (int i = 0; i < len(n->ch); i += 2) {
        if (i)
            qq += ',';
        // TODO: Парсить f string и раскрывать макросы в выражениях
        if (n->ch[i]->term.find('{') != string::npos)
            qq += 'f';
        qq += R"(""")";
        if(!i && pref)
            qq += pref;

        qq += n->ch[i]->term;
        if(i==len(n->ch)-1 && suf)
            qq+=suf;
        qq += R"(""")";
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

ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees){
    if (parts.size() != subtrees.size()+1)
        throw GrammarError("in quasiquote nubmer of string parts = {}, number of subtrees = {}"_fmt(parts.size(), subtrees.size()));
    string qq; // = parts[0];
    for(int i=0; i<len(subtrees); i++) {
        qq += parts[i];
        ((qq += '$') += px->grammar().nts[subtrees[i]->rule_nt()])+=' ';
    }
    qq += parts.back();
    return px->quasiquote(nt, qq, subtrees, QExpr, QStarExpr);
}

void check_quote(ParseContext& px, ParseNodePtr& n){
    if(!px.inQuote())
        throw GrammarError("$<ident> outside of quasiquote");
}

bool endsWidth(const string &x, const std::string& ptr){
    if (x.size() < ptr.size())return false;
    for(size_t i = 0, j = x.size()-ptr.size(); i<ptr.size(); i++, j++)
        if(x[j]!=ptr[i])return false;
    return true;
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
void init_python_grammar(PythonParseContext* px, bool read_by_stmt, const string& syntax_def) {
	const shared_ptr<GrammarState>& pg = px->grammar_ptr();
	ParseContext px0;
	GrammarState& g0 = px0.grammar();

    pg->addNewNTAction([](GrammarState* g, const string& ntn, int nt) {
        // addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), check_quote, QExpr);
   		addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), check_quote, QExpr);
        if (endsWidth(ntn, "_many")) {
            std::string ntbase(ntn.begin(), ntn.end() - 5);
            //if (g->nt.count(ntbase))
            addRule(*g, "{} -> '*${}'"_fmt(ntbase, ntn), check_quote, QStarExpr);
        }
        addRule(*g, "qqst -> '{}`' {} '`'"_fmt(ntn, ntn));
	});
    pg->setWsToken("ws");
    pg->addLexerRule("ws", R"(([ \t\n\r] / comment)*)");

    init_base_grammar(g0, pg.get());
    addRule(g0, "rule_symbol -> '{' rule_rhs '}'", [px](ParseContext& pt, ParseNodePtr& n) {
        vector<vector<string>> v = getVariants(n->ch[0]);
        if(v.size()>1)throw GrammarError("Error in grammar: many cannot be applied to expression containing several variants");
        auto &ntname = px->ntmap[v];
        string manyntname;
        if (ntname.empty()) {
            if(v[0].size()==1) ntname = v[0][0];
            else               ntname = "__nt_{}"_fmt(px->ntmap.size());
            manyntname = ntname+"_many";
            px->grammar().addRule(ntname, v[0]);
            addRule(px->grammar(), "{}_many -> {}"_fmt(ntname, ntname));
            addRule(px->grammar(), "{}_many -> {}_many {}"_fmt(ntname, ntname, ntname), flatten_check);
        } else manyntname = ntname+"_many";
        n->ch[0] = px->newnode().get();
        n->ch[0]->term = manyntname;
        n->ch[0]->nt = px->grammar().ts["ident"];
    });

	g0.addLexerRule("comment", "'#' [^\\n]*");
	string text = syntax_def.empty() ? loadfile("../test/syntax/python.txt") : syntax_def;
	parse(px0, text);

    pg->setStart("text");
	pg->setSOFToken("SOF", -1);
	addRule(*pg, "text -> SOF root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
	    if(read_by_stmt) {
            n.reset(n->ch[0]);
            n->type = ParseNode::Final;
        }
	});
    addRule(*pg, "text -> text root_stmt", [read_by_stmt](ParseContext&, ParseNodePtr& n){
        if(read_by_stmt) {
            n.reset(n->ch[1]);
            n->type = ParseNode::Final;
        }
    });

    pg->addToken("qqopen", "ident '`'");
    pg->addToken("qqmid", "('\\\\' [^] / [^`$] / '$$')*");
    pg->addToken("qqmidfirst", "!'`' qqmid");

    //addRule(*pg, "qqitem -> ident", QQSingle);
    //addRule(*pg, "qqitem -> '{' expr '}'", QQSingle);
    //addRule(*pg, "qqitem -> '*' ident", QQMany);
    //addRule(*pg, "qqitem -> '*{' expr '}'", QQMany);

	addRule(*pg, "qqst -> qqmidfirst"); // qqmid ");
	//addRule(*pg, "qqst -> qqst '$' qqitem qqmid", flatten);
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

    ///////////////////////// new syntax definition rules //////////////////
    addRule(*pg, "syntax_rule -> '(' ident syntax_elems ')'", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
        add_macro_rule(px, n, 0);
    });
    addRule(*pg, "root_stmt -> 'syntax' syntax_rule ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
        flatten(pt, n);
        string fnm = px.pymodule.uniq_name("syntax_" + n[0].term);
        int id = conv_macro(px, n, 0, fnm, "_syntax_rule");
        px.pymodule.syntax[id] = PySyntax{fnm, id};
    });
    addRule(*pg, "root_stmt -> 'syntax_expand' syntax_rule ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
        flatten(pt, n);
        string fnm = px.pymodule.uniq_name("syntax_" + n[0].term);
        int id = conv_macro(px, n, 0, fnm, "_add_pyexpand_rule");
        px.pymodule.syntax[id] = PySyntax{fnm, id};
    });
    addRule(*pg, "root_stmt -> 'defmacro' ident syntax_rule ':' suite", [](ParseContext& pt, ParseNodePtr& n) {
        auto &px = static_cast<PythonParseContext&>(pt);
        flatten_p(pt, n, 1);
        string fnm = px.pymodule.uniq_name("macro_" + n[0].term);
        int id = conv_macro(px, n, 1, fnm, "_macro_rule");
        px.pymodule.macros[id] = PyMacro{ fnm, id };
    });

    /////////////////////////// gimport rules //////////////////////////////
    addRule(*pg, "root_stmt -> 'gimport' dotted_name EOL", [](ParseContext& pt, ParseNodePtr& n){
        auto &px = static_cast<PythonParseContext&>(pt);
        auto s = ast_to_text(&pt, &n[0]);
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        n.reset(px.quasiquote("root_stmt", "_gimport___('{}', None, globals())"_fmt(s), {}, QExpr, QStarExpr));
    });
    addRule(*pg, "root_stmt -> 'gimport' dotted_name 'as' ident EOL", [](ParseContext& pt, ParseNodePtr& n){
        auto &px = static_cast<PythonParseContext&>(pt);
        auto s = ast_to_text(&pt, &n[0]);
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        n.reset(px.quasiquote("root_stmt", "_gimport___('{}', '{}', globals())"_fmt(s, n[1].term), {}, QExpr, QStarExpr));
    });

    ////////////////////////////////////////////////////////////////////////

    // Добавление поддержки специальных токенов в квазицитатах, начинающихся с $$
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

                return num;
            } else {
                int q = pos + 1;
                for (; isalnum(s[q]) || s[q] == '_';)
                    q++;
                string id(s + pos, q - pos);
                int pnum = lex->ctoken_num(id.c_str());
                if (pnum < 0)
                    throw SyntaxError("Invalid token {} at {}"_fmt(Substr{s + pos, q - pos}, lex->cpos()));
                pos = q;

                return pnum;
            }
        }
        return -1;
    });
}

void init_python_grammar_cached(PythonParseContext* px, bool read_by_stmt, const string& syntax_def) {
    static shared_ptr<PythonParseContext> cached, cached_stmt;
    shared_ptr<PythonParseContext>& c = read_by_stmt ? cached_stmt : cached;
    if(!c) {
        c = make_shared<PythonParseContext>();
        init_python_grammar(c.get(), read_by_stmt, syntax_def);
    }
    *px = *c;
}

int add_lexer_rule(PythonParseContext *px, const string&nm, const string&rhs)
{
    if (px->grammar().ts.has(nm))
        throw Exception("Lexer PEG nonterminal {} already exists"_fmt(nm));
    return px->grammar().addLexerRule(nm, rhs);
}

int add_token(PythonParseContext *px, const string& nm, const string& tokdef)
{
    if (px->grammar().ts.has(nm))
        throw Exception("Token {} already exists"_fmt(nm));
    return px->grammar().addToken(nm, tokdef);
}

void del_python_context(PythonParseContext *px) {
    delete px;
}

int add_rule(ParseContext* px, const string& lhs, const string& rhs, int lpr, int rpr) {
    return addRule(((ParseContext*)px)->grammar(), string(lhs) + " -> " + rhs, -1, lpr, rpr);
}

ParserState* new_parser_state(ParseContext *px, const string& text, const string& start) {
    return new ParserState(px, text, start);
}

bool at_end(ParserState *state) {
    return state->atEnd();
}

ParseNode* continue_parse(ParserState *state) {
    ParseTree tree = state->parse_next();
    return tree.root.get();
}

void del_parser_state(ParserState* state) {
    delete state;
}

std::string ast_to_text(ParseContext* pcontext, ParseNode *pn) {
    return tree2str(pn, pcontext->grammar_ptr().get());
}

string get_terminal_str(ParseNode* pn){
    if (!pn->isTerminal())
        throw Exception("get string value of nonterminal");
    return pn->term.c_str();
}

int set_cpp_debug(int dbg) {
    setDebug(dbg);
    return 0;
}

PythonParseContext *create_python_context(bool read_by_stmt, const string &syntax_def) {
    auto px = make_unique<PythonParseContext>();
    init_python_grammar_cached(px.get(), read_by_stmt != 0, syntax_def);
    return px.release();
}

