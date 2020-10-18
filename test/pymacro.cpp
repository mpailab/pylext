#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include "Parser.h"
#include "base.h"
#include "pymacro.h"
using namespace std;

enum MacroRule {
	QExpr = SynTypeLast + 1,
	MacroArg,
	MacroArgExpand,
	MacroConstStr

};

ParseNode* replace_trees_rec(ParseNode* n, const vector<ParseNode*>& nodes, int &pos) {
	if (n->rule_id == QExpr) {
		if (pos >= (int)nodes.size())
			throw ParserError{ n->loc, "not enough arguments for quasiquote" };
		return nodes[pos++];
	}
	for (auto& ch : n->ch)
		ch = replace_trees_rec(ch, nodes, pos);
	return n;
}

ParseNode* replace_trees(ParseNode* n, const vector<ParseNode*>& nodes) {
	int pos = 0;
	return replace_trees_rec(n, nodes, pos);
}

void init_python_grammar(GrammarState& g) {
	g.data = PyMacroModule();
	init_base_grammar(g);
	g.addNewNTAction([](GrammarState* g, const string& ntn, int nt) {
		addRule(*g, ntn + " -> '${' + " + ntn + " +  ':' expr '}'");
		addRule(*g, "expr -> '" + ntn + "`' " + ntn + " '`'");
	});
	addRule(g, "expr -> '$' ident", QExpr);
	addRule(g, "expr -> '${' expr '}'", QExpr);
	addRule(g, "expr -> 'e`' expr '`'");
	string text = loadfile("syntax/python.txt");
	parse(g, text);
	addRule(g, "syntax_elem -> sq_string", MacroConstStr);
	addRule(g, "syntax_elem -> ident ':' ident", MacroArg);
	addRule(g, "syntax_elem -> ident ':' '*' ident", MacroArgExpand);

	addRule(g, "syntax_elems -> ',' syntax_elem");
	addRule(g, "syntax_elems -> syntax_elems ',' syntax_elem", [](GrammarState*, ParseNode& n) {});
	addRule(g, "'syntax' '(' ident syntax_elems ')' ':' suite", [](GrammarState* g, ParseNode& n) {
		PyMacroModule* m = any_cast<PyMacroModule>(&g->data);
		n.rule = 0; /// Извлечь правило 
		vector<string> rhs;
		for (int i = 0; i < (int)n[1].ch.size(); i++)
			if (n[1][i].rule_id == MacroArg || n[1][i].rule_id == MacroArgExpand)
				rhs.push_back(n[1][i][0].term);
			else rhs.push_back(n[1][i].term);
		g->addRule(n[0].term, rhs);
		n[0].term = m->uniq_name("syntax_"+n[0].term);
	});
}
