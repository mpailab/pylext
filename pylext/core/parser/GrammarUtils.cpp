#include "GrammarUtils.h"

#include <utility>

int addRule(GrammarState& gr, const string& s, SemanticAction act, int id, int lpr, int rpr) {
	int pos = 0, i = 0;
	string A;
	vector<string> rhs;
	for (; s[pos]; pos++) {
		if (isspace(s[pos]))continue;
		if (i == 1) {
			if (s[pos] == '=')i++;
			else if (s[pos] == '-' && s[pos + 1] == '>')i++, pos++;
			else throw Exception("Error : '->' expected between left and right parts of rule");
		} else if (isalpha(s[pos]) || s[pos] == '_') {
			int q = pos;
			for (; isalnum(s[q]) || s[q] == '_';)q++;
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
			if (s[q] != '\'')throw Exception("Error : ' expected");
			string tok = s.substr(pos, q - pos + 1);
			rhs.push_back(tok);
			i++;
			pos = q;
		} else throw Exception("Error : unexpected symbol '"s + s[pos] + "'");
	}
	return gr.addRule(A, rhs, std::move(act), id, lpr, rpr);
}

int addRule(GrammarState& gr, const string& s, int id, int lpr, int rpr) {
	return addRule(gr, s, SemanticAction(), id, lpr, rpr);
}

string loadfile(const string& fn) {
	ifstream f(fn);
	if (!f.is_open())throw Exception("cannot open file `" + fn + "`");
	return string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string read_whole_file(const std::string& fn) {
	std::ifstream file(fn, std::ios::binary);
	file.seekg(0, std::istream::end);
	std::size_t size(static_cast<size_t>(file.tellg()));

	file.seekg(0, std::istream::beg);

	std::string result(size, 0);
	file.read(&result[0], size);

	return result;
}

vector<vector<string>> getVariants(ParseNode* n) {
	vector<vector<string>> res;
	while (!n->isTerminal() && n->ch.size() == 1 && n->rule_id < 0)
		n = n->ch[0];
	switch (n->rule_id) {
	case Or: for (auto* m : n->ch)res += getVariants(m);
		return res;
	case Maybe:
		res = getVariants(n->ch[0]);
		res.emplace_back();
		return res;
	case Concat:
		res = { {} };
		for (auto* m : n->ch) {
			auto x = getVariants(m);
			auto t = res; res.clear();
			for (auto& u : t)
				for (auto& v : x)
					res.push_back(u + v);
		}
		return res;
    default:
        if(n->rule_id>0)
            throw Exception("Invalid rule_id flag {}"_fmt(n->rule_id));
	}
	Assert(n->isTerminal());
	return { { n->term } };
}

/// f(f(x1,...,xn),y1,..,ym) -> f(x1,...,xn,y1,...,ym)
void flatten(ParseContext&, ParseNodePtr& n) {
    n[0].ch.insert(n[0].ch.end(), n->ch.begin() + 1, n->ch.end());
    n.reset(&n[0]);
}

void flatten_p(ParseContext&, ParseNodePtr& n, int pos) {
    n[pos].ch.insert(n[pos].ch.begin(), n->ch.begin(), n->ch.begin()+pos);
    n[pos].ch.insert(n[pos].ch.end(), n->ch.begin() + pos + 1, n->ch.end());
    n.reset(&n[pos]);
}

/// f(f(x1,...,xn),y1,..,ym) -> f(x1,...,xn,y1,...,ym)
void flatten_check(ParseContext&, ParseNodePtr& n) {
    if(n[0].flattened && n[0].rule == n->rule) {
        n[0].ch.insert(n[0].ch.end(), n->ch.begin() + 1, n->ch.end());
        n.reset(&n[0]);
    }
    n->flattened = true;
}


void init_base_grammar(GrammarState& st, GrammarState* target) {
	st.setStart("text");
	st.setWsToken("ws");
	st.addLexerRule("ws", R"(([ \t\n\r] / comment)*)");
	//st.addLexerRule("comment", "'#' [^\\n]*");
	st.addToken("ident", "[_a-zA-Z][_a-zA-Z0-9]*");
	st.addLexerRule("peg_concat_expr", R"(ws (([&!] ws)* ws ('(' peg_expr ws ')' / '[' ('\\' [^\n] / [^\]\n])* ']' / sq_string / dq_string / ident) (ws [*+?])*)+)");
	st.addLexerRule("peg_expr", "ws peg_concat_expr (ws '/' ws peg_concat_expr)*");
	st.addToken("sq_string", (R"('\'' ('\\' [^] / [^\n'])* '\'')"));
	st.addToken("dq_string", (R"((ws '"' ('\\' [^] / [^\n"])* '"')+)"));
	st.addToken("syn_int", "[0-9]+");
	st.addToken("peg_expr_def", "'`' ws peg_expr ws '`'");
	//st.addToken("ident", ("\\b[_[:alpha:]]\\w*\\b"));
	//st.addToken("token_def", ("\\(\\?\\:[^#\\n]*\\)(?=\\s*($|#))"));
	//st.addToken("sq_string", ("'(?:[^'\\\\]|\\\\.)*'"));
	addRule(st, "string -> sq_string");
	addRule(st, "string -> dq_string");
	addRule(st, "token_sum -> ident");
	addRule(st, "token_sum -> sq_string");
	addRule(st, "rule_symbol -> token_sum");
	//addRule(st, "rule_symbol -> sq_string");
	addRule(st, "rule_symbol -> '(' rule_rhs ')'");
	addRule(st, "rule_symbol -> '[' rule_rhs ']'", Maybe);

	addRule(st, "rule_rhs_seq -> rule_symbol");
	addRule(st, "rule_rhs_seq -> rule_rhs_seq rule_symbol", Concat);
	addRule(st, "rule_rhs -> rule_rhs_seq");
	addRule(st, "rule_rhs -> rule_rhs_seq '|' rule_rhs", Or);

	addRule(st, "new_syntax_expr -> '%' 'syntax' ':' ident '->' rule_rhs", [target](ParseContext& pt, ParseNodePtr& n) {
		Assert(n[0].isTerminal());
		for (auto& x : getVariants(n->ch[1])) {
			target->addRule(n[0].term, x);
		}
	});
	addRule(st, "new_syntax_expr -> '%' 'infxl' '(' ident ',' syn_int ')' ':' rule_rhs", [target](ParseContext& g, ParseNodePtr& n) {
		Assert(n[0].isTerminal());
		Assert(n[1].isTerminal());
		unsigned pr = atoi(n[1].term.c_str());
		for (auto& x : getVariants(n->ch[2])) {
			x.insert(x.begin(), { n[0].term });
			x.push_back({ n[0].term });
			target->addRuleAssoc(n[0].term, x, pr, 1);
		}
	});
	addRule(st, "new_syntax_expr -> '%' 'infxr' '(' ident ',' syn_int ')' ':' rule_rhs", [target](ParseContext& g, ParseNodePtr& n) {
		Assert(n[0].isTerminal());
		Assert(n[1].isTerminal());
		unsigned pr = atoi(n[1].term.c_str());
		for (auto& x : getVariants(n->ch[2])) {
			x.insert(x.begin(), { n[0].term });
			x.push_back({ n[0].term });
			target->addRuleAssoc(n[0].term, x, pr, -1);
		}
	});
	addRule(st, "new_syntax_expr -> '%' 'postfix' '(' ident ',' syn_int ')' ':' rule_rhs", [target](ParseContext& g, ParseNodePtr& n) {
		Assert(n[0].isTerminal());
		Assert(n[1].isTerminal());
		unsigned pr = atoi(n[1].term.c_str());
		for (auto& x : getVariants(n->ch[2])) {
			x.insert(x.begin(), { n[0].term });
			target->addRuleAssoc(n[0].term, x, pr, 0);
		}
	});
	addRule(st, "new_syntax_expr -> '%' 'prefix' '(' ident ',' syn_int ')' ':' rule_rhs", [target](ParseContext& g, ParseNodePtr& n) {
		Assert(n[0].isTerminal());
		Assert(n[1].isTerminal());
		unsigned pr = atoi(n[1].term.c_str());
		for (auto& x : getVariants(n->ch[2])) {
			x.push_back({ n[0].term });
			target->addRuleAssoc(n[0].term, x, pr, 0);
		}
	});
	addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '=' peg_expr_def", [target](ParseContext& g, ParseNodePtr& n) { target->addLexerRule(n.get(), true); });
	addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '/=' peg_expr_def", [target](ParseContext& g, ParseNodePtr& n) { target->addLexerRule(n.get(), true); });
	addRule(st, "new_syntax_expr -> '%' 'token' ':' ident '\\=' peg_expr_def", [target](ParseContext& g, ParseNodePtr& n) { target->addLexerRule(n.get(), true, true); });

	addRule(st, "new_syntax_expr -> '%' 'pexpr' ':' ident '=' peg_expr_def", [target](ParseContext& g, ParseNodePtr& n) { target->addLexerRule(n.get(), false); });
	addRule(st, "new_syntax_expr -> '%' 'pexpr' ':' ident '/=' peg_expr_def", [target](ParseContext& g, ParseNodePtr& n) { target->addLexerRule(n.get(), false); });

	if (&st != target) {
		addRule(st, "new_syntax_expr -> '%' 'start' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setStart(n[0].term); });
		addRule(st, "new_syntax_expr -> '%' 'ws' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setWsToken(n[0].term); });
		st.addLexerRule("comment", "'#' [^\\n]*");
	}
	addRule(st, "new_syntax_expr -> '%' 'ws' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setWsToken(n[0].term); });
	addRule(st, "new_syntax_expr -> '%' 'indent' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setIndentToken(n[0].term); });
	addRule(st, "new_syntax_expr -> '%' 'dedent' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setDedentToken(n[0].term); });
	addRule(st, "new_syntax_expr -> '%' 'check_indent' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setCheckIndentToken(n[0].term); });
	addRule(st, "new_syntax_expr -> '%' 'eol' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setEOLToken(n[0].term); });
	addRule(st, "new_syntax_expr -> '%' 'eof' ':' ident", [target](ParseContext& g, ParseNodePtr& n) { target->setEOFToken(n[0].term); });

	addRule(st, "new_syntax -> new_syntax_expr ';'");
	addRule(st, "text -> new_syntax text");// , [](ParseContext&, ParseNode&n) { n = ParseNode(move(n[1])); }); // TODO: потенциальная утечка памяти -- проверить !!!

	addRule(st, "new_syntax_expr -> '%' 'stats'", [target](ParseContext& g, ParseNodePtr&) {
		cout << "===================== Grammar statistics ========================" << endl;
		cout << "    Number of constant tokens     :    " << target->lex.ctoken_num() << endl;
		cout << "    Number of NON-constant tokens :    " << target->lex.nctoken_num() << endl;
		cout << "    Number of all tokens          :    " << target->ts._i.size() - 1/*lex.tokens.size() + g->lex.cterms.size()*/ << endl;
		cout << "    Number of non-terminals       :    " << target->nts._i.size() << endl;
		cout << "    Number of productions         :    " << target->rules.size() << endl;
		int l = 0, total = 0;
		for (auto& r : target->rules) {
			l = max(l, (int)r.rhs.size());
			total += (int)r.rhs.size();
		}
		cout << "    Maximum production length     :    " << l << endl;
		cout << "    Average production length     :    " << total * 1. / target->rules.size() << endl;
		cout << "=================================================================" << endl;
	});
	addRule(st, "new_syntax_expr -> '%' 'print'", [target](ParseContext& g, ParseNodePtr&) {
		cout << "=====================    Grammar rules   ========================" << endl;
		target->print_rules(cout);
		cout << "=================================================================" << endl;
		//addRule(st, "text -> new_rule text", [](ParseContext&, ParseNode&n) {n = move(n[1]); });
	});
	addRule(st, "new_syntax_expr -> '%' 'debug' 'on'", [](ParseContext& g, ParseNodePtr&) { setDebug(1); });
	addRule(st, "new_syntax_expr -> '%' 'debug' 'off'", [](ParseContext& g, ParseNodePtr&) { setDebug(0); });
	addRule(st, "text -> new_syntax");
}
