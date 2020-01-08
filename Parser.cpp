#include <iostream>
#include <queue>
#include <sstream>
#include "Parser.h"

bool debug_pr = false;
void setDebug(bool b) { debug_pr = b; }
ParseNode termnode(const Token& t) {
	ParseNode res;
	res.nt = t.type;
	res.loc = t.loc;
	res.term = t.str();
	return res;
}

inline void get_union(NTSet &x, const unordered_map<int, NTSet> &m, int i) {
	auto it = m.find(i);
	if (it != m.end())x |= it->second;
}

bool shift(GrammarState &g, const LR0State &s, LR0State &res, int t, bool term) {
	int sz = (int)s.v.size(), j = 0, k = 0;
	bool r = false;
	for (int i = 0; i < sz; i++) { // Для каждого слоя в текущем состоянии ищем, по какому ребру можно пройти по символу t
		auto &ed = term ? s.v[i].v->termEdges : s.v[i].v->ntEdges;
		auto it = ed.find(t); // TODO: заменить unordered_map на структуру с битовыми масками, по крайней мере, для нетерминалов
		if ((it != ed.end())&&it->second->phi.intersects(s.v[i].M)) { // Проверяем, проходит ли через следующую вершину хотябы одно правило для текущего множества нетерминалов s.v[i].M
			s.v[i].sh = it->second.get(); // Помечаем, что в i-м слое можно идти в следующую вершину
			r = true; // Помечаем, что сдвиг по нетерминалу t возможен
			k++;
		} else s.v[i].sh = 0;
	}
	if (!r)return false;
	decltype(res.v) sv0;
	auto *sv = &s.v;
	if (&res == &s) {
		sv0 = std::move(res.v);
		sv = &sv0;
	}
	res.v.resize(k + 1);
	res.v[k].M.clear();
	res.la.clear();
	for (int i = 0; i < sz; i++) {
		auto &p = (*sv)[i];
		if (p.sh) { // Если переход возможен в i-м слое
			auto *x = res.v[j].v = p.sh; // Добавляем в новый фрейм соответствующий слой
			res.v[j].M = p.M;
			for (auto &nn : x->ntEdges)
				if (nn.second->phi.intersects(p.M)) {
					res.v[k].M |= g.tf.fst[nn.first]; // Добавляем в новый слой те нетерминалы, которые могут начинаться в данной позиции в i-м слое
					LAInfo pla;
					//auto &pla = res.la[nn.first]; // Информация о предпросмотре при свёртке по символу nn.first
					for (int nt : nn.second->phi & p.M) {
						get_union(pla.t, nn.second->next_mt, nt);        // Пополняем множество предпросмотра терминалов (для случая свёртки по символу nn.first)
						get_union(pla.nt, nn.second->next_mnt, nt);      // Пополняем множество предпросмотра нетерминалов (для случая свёртки по символу nn.first)
					}
					auto &q = *nn.second;
					if (q.finalNT.intersects(p.M)) {
						NTSet M;
						for (int i : p.M & q.finalNT)
							M |= g.tf.T[i];   // Вычисляем M -- множество нетерминалов, по которым можно свернуть
						auto &mm = *(&s - p.v->pos); // ????? Может быть здесь индекс на 1 должен отличаться
						for (int x : M) {
							if (auto *y = g.root.nextN(x)) {
								for (int z : y->phi & p.M) { // !!!!!!!! Тройной цикл по множеству допустимых нетерминалов !!!!!!!! По-другому не понятно как (разве что кешировать всё)
									get_union(pla.t, y->next_mt, z);
									get_union(pla.nt, y->next_mnt, z);
								}
							}
							auto &y = mm.la.find(x);
							pla.t |= y.t;
							pla.nt |= y.nt;
							/*auto it = mm.la.find(x);
							if (it != mm.la.end()) { // Достаём информацию о допустимых символах из предыдущих фреймов
								pla.t |= it->second.t;
								pla.nt |= it->second.nt;
							}*/
						}
					}
					if (!pla.nt.empty() || !pla.t.empty())
						res.la[nn.first] |= pla;
				}
			j++;
		}
	}
	if (res.v[k].M.empty())
		res.v.pop_back();
	else {
		res.v[k].v = &g.root;
		// TODO: Сформировать множество допустимых терминалов для созданной вершины
		// Оно должно состоять из:
		//  - множества терминалов, исходящих из вершины
		//  - для каждого нетерминала A из [psi] из терминалов, исходящих из вершины next(r,A) по нетерминалам из M
		//  - для каждого нетерминала A из [psi] из множества с предыдущего уровня для свёртки по A 
		//  - множества терминалов, с которых начинаются нетерминалы, исходящие из текущей вершины + нетерминалы, исходящие из вершины после свёртки по A
	}
	return true;
}


string prstack(GrammarState& g, SStack& ss, PStack& sp, int k=0);

bool reduce1(GrammarState& g, SStack& ss, PStack& sp) {
	int B=-1;
	LR0State* s = &ss.s.back()-1;
	//for (;;) {
		int r = 0;
		const NTTreeNode* u = 0;
		for (auto& p : s->v) {
			if (int r0 = p.v->finalNT.intersects(p.M, &B)) {
				r += r0;
				if (r > 1 || g.tf.T[B].size() > 1)return true;
				u = p.v;
				if (r > 1)return true;
			}
		}
		if (!u)return false;
		if (u->pos > 1) {
			g.reduce(sp.s.data() + sp.s.size() - u->pos, u, B, B);
		} else sp.s.back().nt = B;
		//if (!shift(g, *(s - 1), *s, B, false))
		//	return false;
		return shift(g, *(s - 1), *s, B, false);
	//}
}

bool reduce0(GrammarState& g, SStack& ss, PStack& sp/*, NTSet &M1*/) {
	int B = -1, C = -1;
	//M1.clear();
	for (;ss.s.size();) {
		LR0State* s = &ss.s.back() - 1;
		int r = 0;
		const NTTreeNode* u = 0;
		for (auto& p : s->v) {
			if (p.M.intersects(p.v->next))return true;
			if (int r0 = p.v->finalNT.intersects(p.M, &B)) {
				r += r0;
				if (r > 1|| g.tf.T[B].intersects(p.M,&C)>1)return true;
				u = p.v;
			}
		}
		if (!u)return false;
		if (u->pos > 1) {
			int p = (int)ss.s.size() - u->pos;
			ss.s.resize(p + 1);
			sp.s[p] = g.reduce(sp.s.data() + sp.s.size() - u->pos, u, B, B);
			sp.s.resize(p + 1);
			if(!shift(g, ss.s[p - 1], ss.s[p], B, false))
				return false;
		} else {
			sp.s.back().nt = B;
			if (!shift(g, *(s - 1), *s, B, false))
				return false;
		}
	}
	return true;
}

bool reduce(GrammarState &g, SStack &ss, PStack& sp, int a) {
	LR0State *s = &ss.s.back();

	g.tmp.clear();
	auto& B = g.tmp.B;
	auto& F = g.tmp.F;
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
		for (int A : F[i]) { // Ищем нетерминал, по которому можно свернуть до данного i-го фрейма
			for (auto &p : s->v) {
				auto it = p.v->ntEdges.find(A);
				if (it == p.v->ntEdges.end() || !it->second->phi.intersects(p.M))continue;
				if (!it->second->termEdges.count(a) || !it->second->termEdges[a]->phi.intersects(p.M)) {
					bool ok = false;
					for (auto &q : it->second->ntEdges) {
						if (q.second->phi.intersects(p.M) && g.tf.fst[q.first].intersects(nn)) {
							ok = true;
							break;
						}
					}
					if (!ok)continue;
				}
				if (A0 >= 0)throw RRConflict("at " + g.lex.curr.cpos.str() + " conflict : 2 different ways to reduce by " + g.ts[a] + ": may be NT = "+g.nts[A]+" or "+g.nts[A0], prstack(g, ss, sp));
				A0 = A;
				break;
			}
		}
		if (A0 >= 0) {
			int A00 = A0;

			auto& path = g.tmp.path;
			int k;
			for (int j = i; j > 0; j = k) {
				const NTTreeNode *u = 0;
				for (auto &p : B[j]) { // Перебор по обратным стрелкам из j-го фрейма вверх. Смотрим, по какая из стрелок соответствует свёртке нетерминала A0
					if (p.M.has(A0)) {
						if (u)throw RRConflict("at "+g.lex.curr.cpos.str()+" conflict : 2 different ways to reduce NT="+g.nts[A0]+" : St["+to_string(j)+"] from St["+to_string(k)+"] or St["+to_string(p.i)+"]",prstack(g,ss,sp,j));
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
					if(r > 1)throw RRConflict("at " + g.lex.curr.cpos.str() + " conflict : 2 different ways to reduce by " + g.ts[a] + ": 3", prstack(g, ss, sp));
				} else {
					auto &tinv = g.tf.inv[A0];
					for (int A : F[k]) {
						if ((u0 = u->nextN(A))) {
							if (int r = tinv.intersects(u0->finalNT, &Bb)) {
								if (r > 1 || A1 >= 0) {
									if(A1 >= 0) throw RRConflict("at " + g.lex.curr.cpos.str() + " RR-conflict(4) : 2 different ways to reduce by " + g.ts[a] + ": NT = "+g.nts[A1]+" or "+g.nts[A], prstack(g, ss, sp,k));
									auto v = tinv & u0->finalNT; 
									vector<int> vv(v.begin(), v.end());
									throw RRConflict("at " + g.lex.curr.cpos.str() + " RR-conflict(5) : 2 different ways to reduce by " + g.ts[a] + ": NT = " + g.nts[vv[0]] + " or " + g.nts[vv[1]], prstack(g, ss, sp, k));
								}
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
			bool r = shift(g, ss.s[p - 1], ss.s[p], A00, false);
			Assert(r);
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

ostream& printstate(ostream &os, const GrammarState &g, const LR0State& st, PStack *ps=0) {
	os << "{";
	int i = 0;
	for (auto &x : st.v) {
		vector<string> rhs;
		int k = 0;
		if (i++)os << ", ";
		if (ps && x.v->pos) {
			os << ps->s[ps->s.size() - x.v->pos].loc.beg.str();
		}
		for (int j : x.M)
			if(!x.v->phi.has(j))
				os << (k++ ? ',' : '(') << g.nts[j];
		os << (k ? "!!!" : "(!!!"); k = 0;
		for (int j : x.M & x.v->phi)
			os << (k++ ? "," : "") << g.nts[j];
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
	os<<"}";
	/*if (st.la.size()) {
		os << " LA = {";
		bool fst = true;
		for (auto &x : st.la) {
			if (!fst)os << ", ";
			os << g.nts[x.first] << " -> " << "(T:";
			for (int y : x.second.t)
				os << " " << g.lex.tName(y);
			if (!x.second.nt.empty()) {
				os << "; NT:";
				for (int y : x.second.t)
					os << " " << g.nts[y];
			}
			os << ")";
		}
		os << "}";
	}*/
	return os;
}

string prstack(GrammarState&g, SStack&ss, PStack &sp, int k) {
	stringstream s;
	for (int i = -k-10; i < -1; i++) {
		if ((int)ss.s.size() + i >= 0)
			printstate(s << (i==-k-1 ? "--> ": "    ") << "St[" << (-1 - i) << "] = ", g, ss.s[ss.s.size() + i]) << "\n";
	}
	printstate(s << (!k ? "--> " : "    ") << "Top   = ", g, ss.s.back(), &sp) << "\n";
	return s.str();
}

bool nextTok(GrammarState &g, SStack &ss) { // Определяет множество допустимых токенов, и лексер пытается их прочитать
	NTSet t, nt;
	auto &s = ss.s.back();
	for (auto &p : s.v) {
		for (int n : p.M & p.v->phi) {
			get_union(t, p.v->next_mt, n);
			get_union(nt, p.v->next_mnt, n);
		}
		if (p.v->finalNT.intersects(p.M)) {
			NTSet M;
			for (int i : p.M & p.v->finalNT)
				M |= g.tf.T[i];   // Вычисляем M -- множество нетерминалов, по которым можно свернуть
			auto &mm = *(&s - p.v->pos); // ????? Может быть здесь индекс на 1 должен отличаться
			for (int x : M) {
				if (auto *y = g.root.nextN(x)) {
					for (int z : y->phi & p.M) { // !!!!!!!! Двойной цикл по множеству допустимых нетерминалов на каждом шаге !!!!!!!!
						get_union(t, y->next_mt, z);
						get_union(nt, y->next_mnt, z);
					}
				}
				auto &y = mm.la.find(x);
				t |= y.t;
				nt |= y.nt;
				//auto it = mm.la.find(x);
				//if (it != mm.la.end()) { // Достаём информацию о допустимых символах из предыдущих фреймов
				//	t |= it->second.t;
				//	nt |= it->second.nt;
				//}
			}
		}
	}
	if (debug_pr) {
		cout << "Lookahead: T =";
		for (int x : t)
			cout << " "<<g.lex.tName(x);
		cout << "; NT =";
		for (int n : nt)
			cout << " " << g.nts[n];
		cout << "; AllT =";
	}
	for (int n : nt)
		t |= g.tf.fst_t[n];
	if (debug_pr) {
		for (int x : t)
			cout << " " << g.lex.tName(x);
		cout << "\n";
	}
	return g.lex.go_next(t);
}

ParseNode parse(GrammarState & g, const std::string& text) {
	SStack ss;
	PStack sp;
	LR0State s0;
	s0.v.resize(1);
	s0.v[0].M = g.tf.fst[g.nts[""]];
	s0.v[0].v = &g.root;
	ss.s.emplace_back(std::move(s0));
	for(g.lex.start(text, &g.tf.fst_t[g.start]); !g.lex.atEnd();) {
		for (int ti = 0; ti < len(g.lex.tok()); ti++){
			auto t = g.lex.tok()[ti];
			if (debug_pr) {
				std::cout << "token of type " << g.ts[t.type] << ": `" << t.str() << "` at " << t.loc << endl;
			}
			if (shift(g, ss.s.back(), s0, t.type, true)) {
				if (debug_pr) {
					std::cout << "Shift by " << g.ts[t.type] << ": ";
					printstate(std::cout, g, s0) << "\n";
				}
				ss.s.emplace_back(move(s0));
				g.lex.acceptToken(t);
				sp.s.emplace_back(termnode(t));

				if (!reduce0(g, ss, sp))
					throw SyntaxError("Cannot shift or reduce after terminal " + g.ts[t.type] + " = `" + t.short_str() + "` at " + t.loc.beg.str(), prstack(g, ss, sp));

				nextTok(g, ss);
				//g.lex.go_next();
				break;
			} else {
				bool r = reduce(g, ss, sp, t.type);
				if (!r) {
					if (ti < len(g.lex.tok())-1) {
						if (debug_pr) {
							std::cout << "Retry with same token of type " << g.ts[(&t)[1].type] << endl;
						}
						continue;
					}
					auto &t1 = g.lex.tok()[0];
					throw SyntaxError("Cannot shift or reduce : unexpected terminal " + g.ts[t1.type] + " = `" + t1.short_str() + "` at " + t1.loc.beg.str(), prstack(g, ss, sp));
				}
				if (debug_pr) {
					std::cout << "Reduce by " << g.ts[t.type] << ": "; printstate(std::cout, g, ss.s.back()) << "\n";
				}
				g.lex.acceptToken(t);
				break;
			}
		}
	}
	if (!reduce(g, ss, sp, 0))
		throw SyntaxError("Unexpected end of file",prstack(g,ss,sp));
	Assert(ss.s.size() == 2);
	Assert(sp.s.size() == 1);
	return move(sp.s[0]);
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

void GrammarState::addLexerRule(const string & term, const string & rhs, bool tok, bool to_begin) {
	if (debug_pr)
		std::cout << "!!! Add lexer rule : " << term << " <- " << rhs << "\n";
	//lex.addNCToken(ts[term], regex(re));
	int n = tok ? ts[term] : 0;
	lex.addPEGRule(term, rhs, n, to_begin);
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
			rule.rhs.push_back(RuleElem{ ts[x],true,true,false });                      // Константный терминал
		} else if (ts.num(x) >= 0)rule.rhs.push_back(RuleElem{ ts[x],false,true,true }); // Неконстантный терминал
		else {
			rule.rhs.push_back(RuleElem{ nts[x],false,false,true });                     // Нетерминал
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

		if (r.term) {
			if(!r.cterm) curr->next_mt[rule.A].add(lex.internalNum(r.num)); // Поддержка структуры для вычисления множества допустимых терминалов и нетерминалов для предпросмотра
		} else curr->next_mnt[rule.A].add(r.num);

		if (!r.term)curr->nextnt.add(r.num); // TODO: сделать запоминание позиции, где было изменение
		auto e = (r.term ? curr->termEdges[r.num] : curr->ntEdges[r.num]).get();
		curr = e;
		curr->phi.add(rule.A); // TODO: сделать запоминание позиции, где было изменение
		curr->pos = ++pos;
		if (curr->_frule < 0)curr->_frule = nrule;
	}
	if (!curr->finalNT.add_check(rule.A)) // TODO: сделать запоминание позиции, где добавлено правило
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
		if(!rule.rhs[0].cterm)
			tf.addRuleBeg_t(lex.curr.pos, rule.A, lex.internalNum(rule.rhs[0].num));
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
	if (!curr->finalNT.add_check(rule.A)) // TODO: сделать запоминание позиции, где добавлено правило
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

bool GrammarState::addLexerRule(const ParseNode * tokenDef, bool tok, bool to_begin) {
	if (tokenDef->ch.size() != 2) {
		error("token definition must have 2 nodes");
		return false;
	}
	if (!tokenDef->ch[0].isTerminal() || !tokenDef->ch[1].isTerminal()) {
		error("token definition cannot contain nonterminals");
		return false;
	}
	addLexerRule(tokenDef->ch[0].term, (tokenDef->ch[1].term), tok, to_begin);
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

void ParseNode::del() {
	queue<ParseNode> q;
	q.push(move(*this));
	while (!q.empty()) {
		for (auto& n : q.front().ch)
			q.push(move(n));
		q.front().ch.clear();
		q.pop();
	}
}
