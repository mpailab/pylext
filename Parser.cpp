#include "Parser.h"
#include <iostream>
const bool debug_pr = false;
ParseNode termnode(const Token& t) {
	ParseNode res;
	res.nt = t.type;
	res.loc = t.loc;
	res.term = t.str();
	return res;
}

bool shift(GrammarState &g, const LR0State &s, LR0State &res, int t, bool term) {
	int sz = (int)s.v.size(), j = 0, k = 0;
	bool r = false;
	for (int i = 0; i < sz; i++) {
		auto &ed = term ? s.v[i].v->termEdges : s.v[i].v->ntEdges;
		auto it = ed.find(t);
		if (((it != ed.end()))) {
			s.v[i].sh = it->second.get();
			r = true;
			k++;
		} else s.v[i].sh = 0;
	}
	if (!r)return false;
	decltype(s.v) sv0;
	auto *sv = &s.v;
	if (&res == &s) {
		sv0 = std::move(s.v);
		sv = &sv0;
	}
	res.v.resize(k + 1);
	for (int i = 0; i < sz; i++) {
		if ((*sv)[i].sh) {
			auto *x = res.v[j].v = (*sv)[i].sh;
			res.v[j].M = (*sv)[i].M;
			for (auto &nn : x->ntEdges)
				if (nn.second->phi.intersects((*sv)[i].M))
					res.v[k].M |= g.tf.fst[nn.first];
			j++;
		}
	}
	if (res.v[k].M.empty())
		res.v.pop_back();
	else res.v[k].v = &g.root;
	return true;
}

bool reduce(GrammarState &g, SStack &ss, PStack& sp, int a) {
	LR0State *s = &ss.s.back();
	struct BElem {
		int i;
		const NTTreeNode *v;
		NTSet M;
	};
	dvector<vector<BElem>> B;
	dvector<NTSet> F;
	int mx = 0;
	for (auto &p : s->v) {
		NTSet M1;
		for (int i : p.M & p.v->finalNT)
			M1 |= g.tf.T[i];
		M1 &= p.M;
		if (!M1.empty()) {
			B[p.v->pos].push_back({ 0,p.v,M1 });
			F[p.v->pos] |= M1;
			mx = max(mx, p.v->pos);
		}
	}
	if (!mx)return false;
	auto &nn = g.tFirstMap[a];
	for (int i = 1; i < F.size(); i++) {
		s--;
		if (F[i].empty()) continue;
		int A0 = -1;
		for (int A : F[i]) {
			for (auto &p : s->v) {
				auto it = p.v->ntEdges.find(A);
				if (it == p.v->ntEdges.end() || !it->second->phi.intersects(p.M))continue;
				if (!it->second->termEdges.count(a)) {
					bool ok = false;
					for (auto &q : it->second->ntEdges) {
						if (q.second->phi.intersects(p.M) && g.tf.fst[q.first].intersects(nn)) {
							ok = true;
							break;
						}
					}
					if (!ok)continue;
				}
				if (A0 >= 0)throw RRConflict("conflict : 2 different ways to reduce: 1");
				A0 = A;
				break;
			}
		}
		if (A0 >= 0) {
			int A00 = A0;
			struct PV {
				const NTTreeNode *v;
				int A;
				int B; // reduction: B -> A -> rule
			};
			vector<PV> path;
			int k;
			for (int j = i; j > 0; j = k) {
				const NTTreeNode *u = 0;
				for (auto &p : B[j]) {
					if (p.M.has(A0)) {
						if (u)throw RRConflict("conflict : 2 different ways to reduce: 2");
						u = p.v;
						k = p.i;
					}
				}
				Assert(u);
				const NTTreeNode *u1 = 0, *u0;
				int A1 = -1, Bb;
				if (!k) {
					u1 = u;
					int r = g.tf.inv[A0].intersects(u->finalNT, &Bb);
					if(r > 1)throw RRConflict("conflict : 2 different ways to reduce: 3");
				} else {
					auto &tinv = g.tf.inv[A0];
					for (int A : F[k]) {
						if ((u0 = u->nextN(A))) {
							if (int r = tinv.intersects(u0->finalNT, &Bb)) {
								if (r > 1 || A1 >= 0) throw RRConflict("conflict : 2 different ways to reduce: 4");
								A1 = A;
								u1 = u0;
							}
						}
					}
					Assert(A1 >= 0);
				}
				//if (A0 != Bb)
				//	path.push_back({ g.root.nextN(Bb),A0 });
				path.push_back({ u1,A0,Bb }); // A0 <- Bb <- rule
				A0 = A1;
			}
			for (int j = (int)path.size(); j--;) {
				//ParseNode pn;
				int p = path[j].v->pos;
				sp.s[sp.s.size() - p] = g.reduce(&sp.s[sp.s.size() - p], path[j].v, path[j].B, path[j].A);
				//pn.rule = path[j].v->rule(path[j].A);
				//pn.ch.resize(p);
				//std::move(sp.s.end() - p, sp.s.end(), pn.ch.begin());
				// TODO: добавить позицию в тексте и т.п.
				sp.s.resize(sp.s.size() - p + 1);
				//sp.s.emplace_back(std::move(pn));
			}
			int p = (int)ss.s.size() - i;
			ss.s.resize(p + 1);
			shift(g, ss.s[p - 1], ss.s[p], A00, false);
			return true;
		}
		for (auto &p : s->v) {
			int pos = p.v->pos;
			if (!pos)continue;
			NTSet M1;
			for (int A : F[i] & p.v->nextnt) {
				auto &e = *p.v->ntEdges.find(A)->second;
				if (e.phi.intersects(p.M))
					M1 |= e.finalNT;
			}
			M1 &= p.M;
			for (int i : M1&p.M)
				M1 |= g.tf.T[i];
			M1 &= p.M;
			if (!M1.empty()) {
				B[i + p.v->pos].push_back({ i,p.v,M1 });
				F[i + p.v->pos] |= M1;
				mx = max(mx, i + p.v->pos);
			}
		}
	}
	return false;
}
ostream & operator<<(ostream &s, Location loc) {
	if (loc.beg.line == loc.end.line) {
		if(loc.beg.col == loc.end.col)return s << "(" << loc.beg.line << ":" << loc.beg.col<<")";
		return s << "(" << loc.beg.line << ":" << loc.beg.col << "-" << loc.end.col << ")";
	}
	return s << "("<<loc.beg.line << ":" << loc.beg.col << ")-(" << loc.end.line << ":" << loc.end.col << ")";
}

ostream& printstate(ostream &os, const GrammarState &g, const LR0State& st) {
	os << "{";
	int i = 0;
	for (auto &x : st.v) {
		vector<string> rhs;
		int k = 0;
		if (i++)os << ", ";
		for (int j : x.M)
			os << (k++ ? ',' : '(') << g.nts[j];
		os << ")";
		if (x.v->pos) {
			os << " ->";
			auto &rule = g.rules[x.v->_frule];
			for (int j = 0; j < x.v->pos; j++) {
				os<<" "<<(rule.rhs[j].term ? g.ts[rule.rhs[j].num] : g.nts[rule.rhs[j].num]);
			}
			auto m = x.v->finalNT & x.M;
			if (!m.empty()) {
				k = 0;
				for (int j : m)
					os << (k++ ? ',' : '[') << g.nts[j];
				os << ']';
			}
		}
	}
	return os<<"}";
}
ParseNode parse(GrammarState & g, const std::string& text) {
	SStack ss;
	PStack sp;
	LR0State s0;
	s0.v.resize(1);
	s0.v[0].M = g.tf.fst[g.nts[""]];
	s0.v[0].v = &g.root;
	ss.s.emplace_back(std::move(s0));
	for(g.lex.start(text); !g.lex.atEnd();) {
		const Token& t = g.lex.tok();
		if (debug_pr) {
			std::cout << "token of type " << g.ts[t.type] <<": `" << t.str() << "` at " << t.loc << endl;
		}
		if (shift(g, ss.s.back(), s0, t.type, true)) {
			if (debug_pr) {
				std::cout << "Shift by " << g.ts[t.type] << ": ";
				printstate(std::cout, g, s0) << "\n";
			}
			ss.s.emplace_back(move(s0));
			sp.s.push_back(termnode(t));
			g.lex.go_next();
		} else {
			bool r = reduce(g, ss, sp, t.type);
			if (!r) throw SyntaxError("Cannot shift or reduce : unexpected terminal");
			if (debug_pr) {
				std::cout << "Reduce by " << g.ts[t.type] << ": "; printstate(std::cout, g, ss.s.back()) << "\n";
			}
		}
	}
	if (!reduce(g, ss, sp, 0))
		throw SyntaxError("Unexpected end of file");
	Assert(ss.s.size() == 2);
	Assert(sp.s.size() == 1);
	return sp.s[0];
}

void GrammarState::error(const string & err) {
	cerr << "Error at line "<< lex.curr.cpos.line<<':'<< lex.curr.cpos.col<<" : " << err << "\n";
	_err.push_back(make_pair(lex.curr.cpos, err));
}
/*
void GrammarState::addToken(const string & term, const string & re) {
	if(debug_pr)
		std::cout << "!!! Add token : " << term << " = " << re << "\n";
	lex.addNCToken(ts[term], regex(re));
}*/

void GrammarState::addLexerRule(const string & term, const string & rhs, bool tok) {
	if (debug_pr)
		std::cout << "!!! Add lexer rule : " << term << " <- " << rhs << "\n";
	//lex.addNCToken(ts[term], regex(re));
	int n = tok ? ts[term] : 0;
	lex.addPEGRule(term, rhs, n);
}

bool GrammarState::addRule(const string & lhs, const vector<string>& rhs, SemanticAction act) {
	if (debug_pr) {
		std::cout << "!!! Add rule  : " << lhs << " = ";
		for (auto&x : rhs)std::cout << " " << x;
		std::cout << "\n";
	}
	CFGRule rule;
	rule.A = nts[lhs];
	rule.action = act;
	for (auto &x : rhs) {
		if (x[0] == '\'') {
			if (ts.num(x) < 0) {
				lex.addCToken(ts[x], x.substr(1,x.size()-2));
			}
			rule.rhs.push_back(RuleElem{ ts[x],true,false });                      // Константный терминал
		} else if (ts.num(x) >= 0)rule.rhs.push_back(RuleElem{ ts[x],true,true }); // Неконстантный терминал
		else {
			rule.rhs.push_back(RuleElem{ nts[x],false,true });                     // Нетерминал
		}
	}
	rule.used = 0;
	for (auto &r : rule.rhs)
		if (r.save)rule.used++;
	NTTreeNode *curr = &root;
	int pos = 0;
	int nrule = (int)rules.size();
	for (auto &r : rule.rhs) {
		curr->next.add(rule.A); // TODO: сделать запоминание позиции, где было изменение
		if (!r.term)curr->nextnt.add(r.num); // TODO: сделать запоминание позиции, где было изменение
		auto e = (r.term ? curr->termEdges[r.num] : curr->ntEdges[r.num]).get();
		curr = e;
		curr->phi.add(rule.A); // TODO: сделать запоминание позиции, где было изменение
		curr->pos = ++pos;
		if (curr->_frule < 0)curr->_frule = nrule;
	}
	if (!curr->finalNT.add(rule.A)) // TODO: сделать запоминание позиции, где добавлено правило
		return false; // Если правило уже было в дереве, то выходим
	curr->rules[rule.A] = nrule;
	
	// Добавляем финальную вершину в список для нетерминала rule.A
	if ((int)ntRules.size() <= rule.A)
		ntRules.resize(rule.A + 1);
	ntRules[rule.A].push_back(curr);

	//if ((int)ntFirstMap.size() <= rule.A)
	//	ntFirstMap.resize(rule.A + 1);

	if (rule.rhs[0].term) {
		tf.checkSize(rule.A);
		auto &tmap = tFirstMap[rule.rhs[0].num];
		tFirstMap[rule.rhs[0].num].add(rule.A);
		tFirstMap[rule.rhs[0].num] |= tf.ifst[rule.A];
	} else tf.addRuleBeg(lex.curr.pos, rule.A, rule.rhs[0].num, len(rule.rhs));

	rules.emplace_back(move(rule));

	return true;
}

bool GrammarState::addRule(const CFGRule & rule) {
	NTTreeNode *curr = &root;
	int pos = 0;
	int nrule = (int)rules.size();
	for (auto &r : rule.rhs) {
		curr->next.add(rule.A); // TODO: сделать запоминание позиции, где было изменение
		if (!r.term)curr->nextnt.add(r.num); // TODO: сделать запоминание позиции, где было изменение
		auto e = (r.term ? curr->termEdges[r.num] : curr->ntEdges[r.num]).get();
		curr = e;
		curr->phi.add(rule.A); // TODO: сделать запоминание позиции, где было изменение
		curr->pos = ++pos;
		if (curr->_frule < 0)curr->_frule = nrule;
	}
	if (!curr->finalNT.add(rule.A)) // TODO: сделать запоминание позиции, где добавлено правило
		return false; // Если правило уже было в дереве, то выходим
	curr->rules[rule.A] = nrule;

	// Добавляем финальную вершину в список для нетерминала rule.A
	if ((int)ntRules.size() <= rule.A)
		ntRules.resize(rule.A + 1);
	ntRules[rule.A].push_back(curr);

	//if ((int)ntFirstMap.size() <= rule.A)
	//	ntFirstMap.resize(rule.A + 1);

	if (rule.rhs[0].term) {
		tf.checkSize(rule.A);
		auto &tmap = tFirstMap[rule.rhs[0].num];
		tFirstMap[rule.rhs[0].num].add(rule.A);
		tFirstMap[rule.rhs[0].num] |= tf.ifst[rule.A];
	} else tf.addRuleBeg(lex.curr.pos, rule.A, rule.rhs[0].num, len(rule.rhs));

	rules.emplace_back(move(rule));

	return true;
}

bool GrammarState::addLexerRule(const ParseNode * tokenDef, bool tok) {
	if (tokenDef->ch.size() != 2) {
		error("token definition must have 2 nodes");
		return false;
	}
	if (!tokenDef->ch[0].isTerminal() || !tokenDef->ch[1].isTerminal()) {
		error("token definition cannot contain nonterminals");
		return false;
	}
	addLexerRule(tokenDef->ch[0].term, (tokenDef->ch[1].term), tok);
	return true;
}

bool GrammarState::addRule(const ParseNode * ruleDef) {
	if (ruleDef->ch.size() < 2) {
		error("syntax definition must have at least 2 nodes");
		return false;
	}
	if (!ruleDef->ch[0].isTerminal()) {
		error("left side of syntax definition must be a non-terminal name");
		return false;
	}
	vector<const ParseNode*> curr;
	for (auto i = ruleDef->ch.size(); --i; )
		curr.push_back(&ruleDef->ch[i]);
	vector<string> res;
	while (curr.size()) {
		auto *x = curr.back(); curr.pop_back();
		if (x->isTerminal()) { res.push_back(x->term); }
		else for (auto i = x->ch.size(); i--; )
			curr.push_back(&x->ch[i]);
	}
	addRule(ruleDef->ch[0].term, res);
	return true;
}
