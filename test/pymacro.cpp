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
	MacroArg,
	MacroArgExpand,
	MacroConstStr
};

/** Заменяет листья с rule_id=QExpr, на соответствующие поддеревья */
ParseNode* replace_trees(ParseNode* n, const vector<ParseNode*>& nodes) {
	auto pos = nodes.begin();
	return replace_trees_rec(n, pos, nodes.end(), QExpr);
}

/** Раскрывает определение макроса, заданное в виде дерева разбора
 *  Определение макроса преобразуется в определение функции, раскрывающей этот макрос
 *  При этом в текущий контекст добавляется новое правило, реализуемое этим макросом
 *  @papam px  -- текущий контекст
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
		} else rhs.push_back(ni.term);
	}
	if (arglist.size() > 1)arglist.back() = ')';
	else arglist += ')';
	//n[0].term = m->uniq_name("syntax_"+n[0].term);
	if (!expand.empty()) {
		ParseNode* stmts = n[off+2].ch[0];
		for (auto& arg : expand) {
			stmts = px.quasiquote("stmts1", "{}=syn_expand({})\n${{stmts1:0}}"_fmt(arg,arg), { stmts }, QExpr);
		}
		n->ch[off+2] = px.quasiquote("suite", "\n ${stmts1:0}\n", { stmts }, QExpr);
	}
	n.reset(px.quasiquote("stmt", "def " + fnm + arglist + ": ${suite:0}", { n->ch[off+2] }, QExpr));
	return px.g->addRule(n[off].term, rhs);
}

/// f(f(x1,...,xn),y1,..,ym) -> f(x1,...,xn,y1,...,ym)
void flatten(ParseContext&, ParseNodePtr& n) {
	n[0].ch.insert(n[0].ch.end(), n->ch.begin() + 1, n->ch.end());
	n.reset(&n[0]);
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
        if(n[i].term.find('{')<n[i].term.size()) qq+='f';
        tostr(qq, n[i].term, '"');
    }

    qq += "],[";
    for (int i = 0; i < qqp; i++) {
        if(i)qq+=',';
        qargs[i] = &n[2 * i + 1];
        qq += "$expr";
    }
    qq += ']';

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
    return ParseNodePtr(px.quasiquote(nt, qq, subtrees));
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
void init_python_grammar(GrammarState& g) {
	g.data = PyMacroModule();
	shared_ptr<GrammarState> pg(&g, [](GrammarState*) {});
	GrammarState g0;
	g.addNewNTAction([](GrammarState* g, const string& ntn, int nt) {
		addRule(*g, "{} -> '${}'"_fmt(ntn, ntn), [](ParseContext& px, ParseNodePtr& n){
		    if(!px.inQuote())
		        throw GrammarError(n->ch[0]->term + " outside of quasiquote");
		}, QExpr);
		addRule(*g, "qqst -> '{}`' {} '`'"_fmt(ntn, ntn));
	});
	init_base_grammar(g0, pg);
	g0.addLexerRule("comment", "'#' [^\\n]*");
	string text = loadfile("syntax/python.txt");
	ParseContext px0(&g0);
	parse(px0, text);
	g.addToken("qqopen", "ident '`'");
	g.addToken("qqmid", "('\\\\' [^] / [^`$] / '$$')*");
	g.addToken("qqmidfirst", "!'`' qqmid");
	//g.addToken("qqmid00", "('\\\\' [^] / [^`$])* '$' &ident");
	//g.addToken("qqmid01", "('\\\\' [^] / [^`$])* '${'");
	//g.addToken("qqbeg1", "`qqpr1");
	//g.addToken("qqbeg0", "`qqpr0");
	//g.addToken("qqmid11", "'}' qqmid01");
	//g.addToken("qqmid10", "'}' qqmid00");
	addRule(g, "qqst -> qqmidfirst"); // qqmid ");
	addRule(g, "qqst -> qqst '$' ident qqmid", flatten);
	addRule(g, "qqst -> qqst '${' expr '}' qqmid", flatten);
	addRule(g, "expr -> '`' qqst '`'", make_qq);
    addRule(g, "expr -> '``' qqst '``'", make_qq);
    addRule(g, "expr -> '```' qqst '```'", make_qq);
    addRule(g, "expr -> ident '`' qqst '`'", make_qqi);
    addRule(g, "expr -> ident '``' qqst '``'", make_qqi);
    addRule(g, "expr -> ident '```' qqst '```'", make_qqi);

	addRule(g, "syntax_elem -> sq_string", MacroConstStr);
	addRule(g, "syntax_elem -> ident ':' ident", MacroArg);
	addRule(g, "syntax_elem -> ident ':' '*' ident", MacroArgExpand);

	addRule(g, "syntax_elems -> ',' syntax_elem");
	addRule(g, "syntax_elems -> syntax_elems ',' syntax_elem", flatten);
	addRule(g, "root_stmt -> 'syntax' '(' ident syntax_elems ')' ':' suite", [](ParseContext& px, ParseNodePtr& n) {
		PyMacroModule* m = any_cast<PyMacroModule>(&px.g->data);
		string fnm = m->uniq_name("syntax_" + n[0].term);
		int id = conv_macro(px, n, 0, fnm);
		m->syntax[id] = PySyntax{fnm, id};
	});
	addRule(g, "root_stmt -> 'defmacro' ident '(' ident syntax_elems ')' ':' suite", [](ParseContext& px, ParseNodePtr& n) {
		PyMacroModule* m = any_cast<PyMacroModule>(&px.g->data);
		string fnm = m->uniq_name("macro_" + n[0].term);
		int id = conv_macro(px, n, 0, fnm);
		m->macros[id] = PyMacro{ fnm, id };
	});
}
