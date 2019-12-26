#include <iostream>
#include <fstream>
#include "Parser.h"
using namespace std;

bool addRule(GrammarState &gr, const string &s, SemanticAction act=SemanticAction()) {
	int pos = 0, i=0;
	string A;
	vector<string> rhs;
	for (; s[pos]; pos++) {
		if (isspace(s[pos]))continue;
		if (i == 1) {
			if (s[pos] == '=')i++;
			else if (s[pos] == '-'&&s[pos + 1] == '>')i++, pos++;
			else throw Exception("Error : '->' expected between left and right parts of rule");
		} else if (isalpha(s[pos])||s[pos]=='_') {
			int q = pos;
			for (; isalnum(s[q])||s[q]=='_';)q++;
			string tok = s.substr(pos, q - pos);
			if (!i)A = tok;
			else rhs.push_back(tok);
			i++;
			pos = q - 1;
		} else if (i > 0 && s[pos] == '\'') {
			int q = pos;
			for (q++; s[q] && s[q] != '\''; q++) {
				if (s[q] == '\\')q++;
			}
			if(s[q]!='\'')throw Exception("Error : ' expected");
			string tok = s.substr(pos, q - pos + 1);
			rhs.push_back(tok);
			i++;
			pos = q;
		} else throw Exception("Error : unexpected symbol '"s + s[pos] + "'");
	}
	gr.addRule(A, rhs, act);
	return true;
}

string loadfile(const string &fn) {
	ifstream f(fn);
	if (!f.is_open())throw Exception("cannot open file `"+fn+"`");
	return string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

#include <chrono>  // for high_resolution_clock

struct Timer {
	decltype(std::chrono::high_resolution_clock::now()) _t0;
	bool _started = false;
	string name;
	Timer(string nm=""):name(nm) {}
	void start() {
		_t0 = std::chrono::high_resolution_clock::now();
		_started = true;
	}
	double stop(){
		_started = false;
		auto _t1 = std::chrono::high_resolution_clock::now();
		return chrono::duration<double>(_t1 - _t0).count();
	}
	double stop_pr() {
		double t = stop();
		if (name.empty())cout << "Time: " << t << " s\n";
		else cout << name << " time: " << t << " s\n";
		return t;
	}
	~Timer() {
		if (_started) stop_pr();
	}
};

int main(int argc, char*argv[]) {
#ifndef _DEBUG
	try {
#endif
		GrammarState st;
		st.setNtNames("text", "new_token", "new_rule");
		st.addLexerRule("ws", "([ \\t\\n\\r] / comment)*");
		st.addLexerRule("comment", "'#' [^\\n]*");
		st.addToken("ident", "[_a-zA-Z][_a-zA-Z0-9]*");
		st.addLexerRule("peg_concat_expr", "ws (([&!] ws)* ws ('(' peg_expr ws ')' / '[' ('\\\\]' / [^\\]\\n])* ']' / sq_string / dq_string / ident) (ws [*+?])*)+");
		st.addLexerRule("peg_expr", "ws peg_concat_expr (ws '/' ws peg_concat_expr)*");
		st.addToken("sq_string", ("'\\'' ('\\\\' [^\\n] / [^\\n'])* '\\''"));
		st.addToken("dq_string", ("(ws '\"' ('\\\\' [^\\n] / [^\\n\"])* '\"')+"));
		st.addToken("peg_expr_def", "'`' ws peg_expr ws '`'");
		//st.addToken("ident", ("\\b[_[:alpha:]]\\w*\\b"));
		//st.addToken("token_def", ("\\(\\?\\:[^#\\n]*\\)(?=\\s*($|#))"));
		//st.addToken("sq_string", ("'(?:[^'\\\\]|\\\\.)*'"));
		addRule(st, "string -> sq_string");
		addRule(st, "string -> dq_string");
		addRule(st, "rule_symbol -> ident"); 
		addRule(st, "rule_symbol -> sq_string");
		addRule(st, "rule_rhs -> rule_symbol");
		addRule(st, "rule_rhs -> rule_rhs rule_symbol");

		addRule(st, "new_syntax -> '%' 'syntax' ':' ident '->' rule_rhs ';'", [](GrammarState*g, ParseNode&n) { g->addRule(&n); });
	
		addRule(st, "new_syntax -> '%' 'token' ':' ident '=' peg_expr_def ';'", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, true); });
		addRule(st, "new_syntax -> '%' 'token' ':' ident '/=' peg_expr_def ';'", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, true); });

		addRule(st, "new_syntax -> '%' 'pexpr' ':' ident '=' peg_expr_def ';'", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, false); });
		addRule(st, "new_syntax -> '%' 'pexpr' ':' ident '/=' peg_expr_def ';'", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, false); });

		addRule(st, "text -> new_syntax text", [](GrammarState*, ParseNode&n) { n = ParseNode(move(n[1])); });
		//addRule(st, "text -> new_rule text", [](GrammarState*, ParseNode&n) {n = move(n[1]); });

		cout << "Const rules:\n";
		st.print_rules(cout);
		cout << "\n";

		string text;
		for (int i = 1; i < argc; i++) {
			text += loadfile(argv[i]);
		}
		Timer tm("Parsing");
		tm.start();
		ParseNode res = parse(st, text);
		tm.stop_pr();
		cout << "Parser finished successfully\n";
		//st.print_rules(cout);
#ifndef _DEBUG
	} catch (Exception &e) {
		cout << "Exception: " << e.what() << "\n";
	}
#endif
	return 0;
}
