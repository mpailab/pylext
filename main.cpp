#include <iostream>
#include <filesystem>
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

std::string read_whole_file(const std::string &fn) {
	std::ifstream file(fn, std::ios::binary);
	file.seekg(0, std::istream::end);
	std::size_t size(static_cast<size_t>(file.tellg()));

	file.seekg(0, std::istream::beg);

	std::string result(size, 0);
	file.read(&result[0], size);

	return result;
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
		if (_started&&name.size()) stop_pr();
	}
};


using namespace filesystem;
int testDir(GrammarState &g, const string& dir, const string &logfile, const string &failed = "failed.txt") {
	ofstream log(logfile);
	ofstream fail(failed);
	int total = 0, success = 0, skip = 0;
	//setDebug(true);
	cout << "\n=========================================================================\n";
	for (auto it = directory_iterator(path(dir)); it != directory_iterator(); ++it) {
		const directory_entry& e = *it;
		if (!e.is_regular_file())continue;
		try {
			string text = read_whole_file(e.path().string());
			bool unicode = false;
			for (char c : text)if (c <= 0)unicode = true;
			if (0&&unicode) {
				skip++;
				continue;
			} else total++;
			Timer tm;
			tm.start();
			auto N = parse(g, text);
			double t = tm.stop();
			cout << setprecision(3);
			log << e.path().filename() << ":\t Success :\t " << N.root->size << " nodes\t time = " << t << "\t(" << text.size() / t * 1e-6 << " MB/s)\n";
			cout << e.path().filename() << ":\t Success :\t " << N.root->size << " nodes\t time = " << t << "\t(" << text.size() / t * 1e-6 << " MB/s)\n";
			success++;
		} catch (SyntaxError & ex) {
			log << e.path().filename() << ":\t Failed  :\t" << ex.what()<<"\n";
			cout << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			fail << e.path().filename() << ":\t" << ex.what() << "\n" << ex.stack_info << "\n";
			fail.flush();
		} catch (Exception & ex) {
			log << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			cout << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			fail << e.path().filename() << ":\t" << ex.what() << "\n";
			fail.flush();
		}
		log.flush();
	}
	cout << "=========================================================================\n\n";
	if (skip) cout << skip << " contain unicode or zero symbols and skipped\n";
	cout << success << " / " << total << " (" << success * 100. / total << "%) passed\n" << (total - success) << " failed\n\n";
	return 0;
}


int main(int argc, char*argv[]) {
#ifndef _DEBUG
	try {
#endif
		GrammarState st;
		st.setStart("text");// , "new_token", "new_rule");
		st.setWsToken("ws");
		st.addLexerRule("ws", "([ \\t\\n\\r] / comment)*");
		//st.addLexerRule("comment", "'#' [^\\n]*");
		st.addToken("ident", "[_a-zA-Z][_a-zA-Z0-9]*");
		st.addLexerRule("peg_concat_expr", "ws (([&!] ws)* ws ('(' peg_expr ws ')' / '[' ('\\\\' [^\\n] / [^\\]\\n])* ']' / sq_string / dq_string / ident) (ws [*+?])*)+");
		st.addLexerRule("peg_expr", "ws peg_concat_expr (ws '/' ws peg_concat_expr)*");
		st.addToken("sq_string", ("'\\'' ('\\\\' [^] / [^\\n'])* '\\''"));
		st.addToken("dq_string", ("(ws '\"' ('\\\\' [^] / [^\\n\"])* '\"')+"));
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

		addRule(st, "new_syntax_expr -> '%' 'syntax' ':' ident '->' rule_rhs", [](GrammarState*g, ParseNode&n) { g->addRule(&n); });
	
		addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '=' peg_expr_def", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, true); });
		addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '/=' peg_expr_def", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, true); });
		addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '\\=' peg_expr_def", [](GrammarState* g, ParseNode& n) { g->addLexerRule(&n, true,true); });

		addRule(st, "new_syntax_expr -> '%' 'pexpr' ':' ident '=' peg_expr_def", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, false); });
		addRule(st, "new_syntax_expr -> '%' 'pexpr' ':' ident '/=' peg_expr_def", [](GrammarState*g, ParseNode&n) { g->addLexerRule(&n, false); });
		addRule(st, "new_syntax -> new_syntax_expr ';'");
		addRule(st, "text -> new_syntax text", [](GrammarState*, ParseNode&n) { n = ParseNode(move(n[1])); });
		addRule(st, "new_syntax_expr -> '%' 'stats'", [](GrammarState* g, ParseNode&) {
			cout << "===================== Grammar statistics ========================" << endl;
			cout << "    Number of constant tokens     :    " << g->lex.cterms.size() << endl;
			cout << "    Number of NON-constant tokens :    " << g->lex.tokens.size() << endl;
			cout << "    Number of all tokens          :    " << g->ts._i.size()-1/*lex.tokens.size() + g->lex.cterms.size()*/ << endl;
			cout << "    Number of non-terminals       :    " << g->nts._i.size() << endl;
			cout << "    Number of productions         :    " << g->rules.size() << endl;
			int l = 0, total = 0;
			for (auto& r : g->rules) {
				l = max(l, (int)r.rhs.size());
				total += (int)r.rhs.size();
			}
			cout << "    Maximum production length     :    " << l << endl;
			cout << "    Average production length     :    " << total*1./g->rules.size() << endl;
			cout << "=================================================================" << endl;
			});
		//addRule(st, "text -> new_rule text", [](GrammarState*, ParseNode&n) {n = move(n[1]); });

		cout << "Const rules:\n";
		st.print_rules(cout);
		cout << "\n";
		string text, dir;
		//setDebug(true);
		if (argc>1 && (argv[1] == "-d"s || argv[1] == "--dir"s)) {
			if (argc < 3)throw Exception("dir argument expected");
			dir = argv[2];
			argv += 2;
			argc -= 2;
		}
		for (int i = 1; i < argc; i++) {
			text += loadfile(argv[i]);
		}
		if (!dir.empty()) {
			addRule(st, "text -> new_syntax");
		}
		if (!text.empty()) {
			Timer tm("Parsing");
			tm.start();
			ParseTree res = parse(st, text);
			tm.stop_pr();
			cout << "Parser finished successfully\n";
		}
		if (!dir.empty()) {
			return testDir(st, dir, "log.txt");
		}
		//st.print_rules(cout);
#ifndef _DEBUG
	} catch (SyntaxError &ex) {
		cout << "Error: " << ex.what() << "\n" << ex.stack_info << "\n";
	} catch (Exception & e) {
		cout << "Exception: " << e.what() << "\n";
	}
#endif
	return 0;
}
