#include "Parser.h"
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <fstream>
#include <utility>
#include "format.h"

int debug_pr = false;

void setDebug(int b) {
    debug_pr = b;
}

ParseNodePtr termnode(const Token& t, ParseContext &pt) {
	ParseNodePtr res = pt.newnode();// std::make_unique<ParseNode>();
	res->nt = t.type;
	res->loc = t.loc;
	res->term = t.str();
	return res;
}

struct PrintState {
	int indent=0;
	int tab = 4;
	void endl(ostream& os) const {
		os << "\n";
		os << string(indent * tab, ' ');
	}
};

struct S {
	vector<unsigned char>& v;
	explicit S(vector<unsigned char>& vv) :v(vv) {}
	template<class I, class=std::enable_if_t<std::is_integral_v<I>>>
	S& operator<<(I x) {
		v.resize(v.size() + sizeof(x));
		*((I*)(v.data() + v.size()) - 1) = x;
		return *this;
	}
	S& operator<<(const Pos& pos) {
		return *this << pos.col << pos.line;
	}
	S& operator<<(const Location& loc) {
		return *this << loc.beg << loc.end;
	}
	S& operator<<(const string& s) {
		*this << (int)s.size();
		size_t sz = v.size();
		v.resize(sz + s.size());
		memcpy(v.data() + sz, s.c_str(), s.size());
		return *this;
	}
	S& operator<<(ParseNode* n) {
		*this << n->nt << n->B << n->rule << n->rule_id << n->loc << n->lpr << n->rpr << n->term<<n->ch.size();
		for (ParseNode* x : n->ch)
			(*this) << x;
		return *this;
	}
};

void ParseNode::serialize(vector<unsigned char>& res) {
	S s(res);
	s << this;
}

void printSpecial(ostream& os, int tn, GrammarState* g, PrintState& pst) {
	if (tn == g->lex.indent) {
		pst.indent++;
		pst.endl(os);
	} else if (tn == g->lex.dedent) {
		pst.indent--;
		pst.endl(os);
	} else if (tn == g->lex.eol) {
		pst.endl(os);
	}
}

void printTerminal(std::ostream& os, int t, const string &tok, GrammarState* g, PrintState& pst) {
	if (tok.empty() && t < g->lex.ctokens.size() && !g->lex.ctokens[t].empty()) {
		os << g->lex.ctokens[t] << " ";
		return;
	}
	int tn = g->lex.internalNum(t);
	if (g->lex.special.has(tn)) {
		printSpecial(os, tn, g, pst);
	} else if (tok.empty() && g->lex.composite.has(tn)) {
		for (auto& x : g->lex.compTokens[tn].t) {
			if (x.first) {
				if (g->lex.special.has(x.second)) {
					printSpecial(os, tn, g, pst);
				}
				cerr << "nonconst token, part of composite token" << endl;
			} else {
				printTerminal(os, x.second, string(), g, pst);
				//os << g->lex.ctokens[x.second] << " ";
			}
		}
	} else
		os << tok << " ";
}

void tree2str_rec(std::ostream& os, ParseNode* n, GrammarState* g, PrintState& pst) {
	if (n->isTerminal()) {
		return printTerminal(os, n->nt, n->term, g, pst);
	}
	auto& r = g->rules[n->rule];
	int pos = 0;
	for (auto& x : r.rhs) {
		if (x.save) {
			tree2str_rec(os, n->ch[pos], g, pst);
			pos++;
		} else {
			if (!x.term) {
				g->print_rule(cerr << "Error: nonterminal " << g->nts[x.num] << " should be saved in ", r);
				cerr << "\n";
				continue;
			}
			if (!x.cterm) {
				g->print_rule(cerr << "Error: nonconst terminal " << g->ts[x.num] << " should be saved in ", r);
				cerr << "\n";
				continue;
			}
			printTerminal(os, x.num, "", g, pst);
		}
	}
}

void print_tree(std::ostream& os, ParseTree &t, GrammarState* g) {
	PrintState pst;
	tree2str_rec(os, t.root.get(), g, pst);
}

string tree2str(ParseTree& t, GrammarState* g) {
	stringstream ss;
	print_tree(ss, t, g);
	return ss.str();
}

void tree2file(const string &fn, ParseTree& t, GrammarState* g) {
	ofstream f(fn);
	print_tree(f, t, g);
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
					LAInfo pla;                       // Информация о предпросмотре при свёртке по символу nn.first
					for (int nt : nn.second->phi & p.M) {
						get_union(pla.t, nn.second->next_mt, nt);        // Пополняем множество предпросмотра терминалов (для случая свёртки по символу nn.first)
						get_union(pla.nt, nn.second->next_mnt, nt);      // Пополняем множество предпросмотра нетерминалов (для случая свёртки по символу nn.first)
					}
					auto &q = *nn.second;
					if (q.finalNT.intersects(p.M)) {
						NTSet M;
						for (int ii : p.M & q.finalNT)
							M |= g.tf.T[ii];   // Вычисляем M -- множество нетерминалов, по которым можно свернуть
						auto &mm = *(&s - p.v->pos); // ????? Может быть здесь индекс на 1 должен отличаться
						for (int w : M) {
							if (auto *y = g.root.nextN(w)) {
								for (int z : y->phi & p.M) { // !!!!!!!! Тройной цикл по множеству допустимых нетерминалов !!!!!!!! По-другому не понятно как (разве что кешировать всё)
									get_union(pla.t, y->next_mt, z);
									get_union(pla.nt, y->next_mnt, z);
								}
							}
							auto &y = mm.la.find(w);
							pla.t |= y.t;
							pla.nt |= y.nt;
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


string prstack(GrammarState& g, const SStack& ss, const PStack& sp, int k=0);

bool reduce1(SStack& ss, PStack& sp, ParseContext &pt) {
	int B = -1;
	GrammarState& g = *pt.g;
	LR0State* s = &ss.s.back() - 1;
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
		g.reduce(sp.s.data() + sp.s.size() - u->pos, u, B, B, pt);
	} else sp.s.back()->nt = B;
	return shift(g, *(s - 1), *s, B, false);
}

bool reduce0(SStack& ss, PStack& sp/*, NTSet &M1*/, ParseContext &pt) {
	int B = -1, C = -1;
	GrammarState& g = *pt.g;
	for (;!ss.s.empty();) {
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
			sp.s[p] = g.reduce(sp.s.data() + sp.s.size() - u->pos, u, B, B, pt);
			sp.s.resize(p + 1);
			if(!shift(g, ss.s[p - 1], ss.s[p], B, false))
				return false;
		} else {
			sp.s.back()->nt = B;
			if (!shift(g, *(s - 1), *s, B, false))
				return false;
		}
	}
	return true;
}

// template<bool finish>
bool reduce(SStack &ss, PStack& sp, LexIterator& lit, int a, ParseContext &pt) {
	LR0State *s = &ss.s.back();
	GrammarState& g = *pt.g;
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
	auto &nn = g.tFirstMap[a]; // finish ? 0 : a];
	for (int i = 1; i < F.size(); i++) {
		s--;
		if (F[i].empty()) continue;
		int A0 = -1;
		for (int A : F[i]) { // Ищем нетерминал, по которому можно свернуть до данного i-го фрейма
			for (auto &p : s->v) {
				auto it = p.v->ntEdges.find(A);
				if (it == p.v->ntEdges.end() || !it->second->phi.intersects(p.M))continue;
				auto iit = it->second->termEdges.find(a);
				if (iit == it->second->termEdges.end() || !iit->second->phi.intersects(p.M)) {
					bool ok = false;
					for (auto &q : it->second->ntEdges) {
						if (q.second->phi.intersects(p.M) && g.tf.fst[q.first].intersects(nn)) {
							ok = true;
							break;
						}
					}
					if (!ok)continue;
				}
				if (A0 >= 0)
				    throw RRConflict("at {} conflict : 2 different ways to reduce by {}: may be NT = {} or {}"_fmt(
                            lit.cpos, g.ts[a], g.nts[A], g.nts[A0]
				        ), prstack(g, ss, sp));
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
						if (u)
						    throw RRConflict("at {} conflict : 2 different ways to reduce NT={} : St[{}] from St[{}] or St[{}]"_fmt(
                                    lit.cpos, g.nts[A0], j, k, p.i
						        ),prstack(g,ss,sp,j));
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
					if(r > 1)
					    throw RRConflict("at {} conflict : 2 different ways to reduce by {}: 3"_fmt(lit.cpos,g.ts[a]), prstack(g, ss, sp));
				} else {
					auto &tinv = g.tf.inv[A0];
					for (int A : F[k]) {
						if ((u0 = u->nextN(A))) {
							if (int r = tinv.intersects(u0->finalNT, &Bb)) {
								if (r > 1 || A1 >= 0) {
									if(A1 >= 0) throw RRConflict("at {} RR-conflict(4) : 2 different ways to reduce by {}: NT = {} or {}"_fmt(lit.cpos, g.ts[a], g.nts[A1], g.nts[A]), prstack(g, ss, sp,k));
									auto v = tinv & u0->finalNT;
									vector<int> vv(v.begin(), v.end());
									throw RRConflict("at {} RR-conflict(5) : 2 different ways to reduce by {}: NT = {} or {}"_fmt(lit.cpos, g.ts[a], g.nts[vv[0]], g.nts[vv[1]]), prstack(g, ss, sp, k));
								}
								A1 = A;
								u1 = u0;
							}
						}
					}
					Assert(A1 >= 0);
				}
				path.push_back({ u1,A0,Bb }); // A0 <- Bb <- rule
				A0 = A1;
			}
			for (int j = (int)path.size(); j--;) {
				int p = path[j].v->pos;
				sp.s[sp.s.size() - p] = g.reduce(&sp.s[sp.s.size() - p], path[j].v, path[j].B, path[j].A, pt);
				// TODO: добавить позицию в тексте и т.п.
				sp.s.resize(sp.s.size() - p + 1);
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
			for (int j : M1&p.M)
				M1 |= g.tf.T[j];
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

ostream& printstate(ostream &os, const GrammarState &g, const LR0State& st, const PStack *ps=0) {
	os << "{";
	int i = 0;
	for (auto &x : st.v) {
		vector<string> rhs;
		int k = 0;
		if (i++)os << ", ";
		if (ps && x.v->pos) {
			os << ps->s[ps->s.size() - x.v->pos]->loc.beg.str();
		}
		for (int j : x.M)
			if(!x.v->phi.has(j))
				os << (k++ ? ',' : '(') << g.nts[j];
		os << (k ? "!!!" : "(!!!");
		k = 0;
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
	return os;
}

string prstack(GrammarState&g, const SStack&ss, const PStack &sp, int k) {
	stringstream s;
	for (int i = -k-10; i < -1; i++) {
		if ((int)ss.s.size() + i >= 0)
			printstate(s << (i==-k-1 ? "--> ": "    ") << "St[" << (-1 - i) << "] = ", g, ss.s[ss.s.size() + i]) << "\n";
	}
	printstate(s << (!k ? "--> " : "    ") << "Top   = ", g, ss.s.back(), &sp) << "\n";
	return s.str();
}

bool nextTok(GrammarState &g, LexIterator& it, SStack &ss) { // Определяет множество допустимых токенов, и лексер пытается их прочитать
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
	return it.go_next(t);
}

ParseTree parse(ParseContext &pt, const std::string& text, const string& start) {
	SStack ss;
	PStack sp;
	//ParseTree pt;
	GrammarState& g = *pt.g;
	LR0State s0;
	if (!pt.error_handler)
		pt.error_handler = std::make_shared<ParseErrorHandler>();
	s0.v.resize(1);
	if (!g.nts.has(start))
		throw GrammarError("grammar doesn't have start nonterminal `" + start + "`");
	auto start_nt = (!start.empty() ? g.getStartNT(start) : g.nts[start]);
	s0.v[0].M = g.tf.fst[start_nt];
	s0.v[0].v = &g.root;
	ss.s.emplace_back(std::move(s0));
	try {
        LexIterator lit(&g.lex, text, &g.tf.fst_t[start_nt]);
		while (!lit.atEnd()) {
			for (int ti = 0; ti < len(lit.tok()); ti++) {
				auto t = lit.tok()[ti];
				if (debug_pr) {
					std::cout << "token of type " << g.ts[t.type] << ": `" << t.str() << "` at " << t.loc << endl;
				}
				if (shift(g, ss.s.back(), s0, t.type, true)) {
					if (debug_pr)
						printstate(std::cout << "Shift by " << g.ts[t.type] << ": ", g, s0) << "\n";

					ss.s.emplace_back(move(s0));
					lit.acceptToken(t);

					sp.s.emplace_back(termnode(t, pt));

					//if (!reduce0(g, ss, sp, pt))
					//	throw SyntaxError("Cannot shift or reduce after terminal " + g.ts[t.type] + " = `" + t.short_str() + "` at " + t.loc.beg.str(), prstack(g, ss, sp));

					nextTok(g, lit, ss);
					//g.lex.go_next();
					break;
				} else {
					bool r = reduce(ss, sp, lit, t.type, pt);
					if (!r) {
						if (ti < len(lit.tok()) - 1) {
							if (debug_pr)
								std::cout << "Retry with same token of type " << g.ts[lit.tok()[ti + 1].type] << endl;

							continue;
						}
						auto &t1 = lit.tok()[0];
						auto hint = pt.error_handler->onNoShiftReduce(&g, ss, sp, t1);
						if (hint.type == ParseErrorHandler::Hint::Uncorrectable)
							throw SyntaxError();
						throw SyntaxError("Cannot shift or reduce : unexpected terminal {} = `{}` at {}"_fmt(g.ts[t1.type], t1.short_str(), t1.loc.beg), prstack(g, ss, sp));
						//TODO: use hint instead of throwing exception
					}
					if (debug_pr)
                        printstate(std::cout << "Reduce by " << g.ts[t.type] << ": ", g, ss.s.back()) << "\n";

					lit.acceptToken(t);
					break;
				}
			}
		}
		if (!reduce(ss, sp, lit, 0, pt))
			throw SyntaxError("Unexpected end of file");
	} catch (SyntaxError &e) {
		if (e.stack_info.empty()) {
			e.stack_info = prstack(g, ss, sp);
		}
		throw move(e);
	}
	Assert(ss.s.size() == 2);
	Assert(sp.s.size() == 1);
	ParseTree tree;
	tree.root = sp.s[0];
	return tree;
}

ParserState::ParserState(ParseContext *px, std::string txt, const string &start): pt(px), lit(&pt->g->lex, std::move(txt)) {
    //cout<<"at "<<__LINE__<<endl;
    g = pt->g;
    //cout<<"at "<<__LINE__<<endl;
    if (!pt->error_handler)
        pt->error_handler = std::make_shared<ParseErrorHandler>();
    s0.v.resize(1);
    if (!g->nts.has(start))
        throw GrammarError("grammar doesn't have start nonterminal `" + start + "`");
    int nt_start = g->nts[start];
    s0.v[0].M = g->tf.fst[nt_start];
    s0.v[0].v = &g->root;
    ss.s.emplace_back(std::move(s0));
    //cout<<"at "<<__LINE__<<endl;

    lit.start(&g->tf.fst_t[nt_start]);
    //cout<<"at "<<__LINE__<<endl;
}

ParseTree ParserState::parse_next() {
    try {
        cout<<"Start parse: state = "<<state<<endl;
        switch(state){
            case AtStart: break;
            case Paused: goto resume;
            case AtEnd: return {};
        }
        while (!lit.atEnd()) {
            for (int ti = 0; ti < len(lit.tok()); ti++) {
                auto t = lit.tok()[ti];
                if (debug_pr) {
                    std::cout << "token of type " << g->ts[t.type] << ": `" << t.str() << "` at " << t.loc << endl;
                }
                if (shift(*g, ss.s.back(), s0, t.type, true)) {
                    if (debug_pr)
                        printstate(std::cout << "Shift by " << g->ts[t.type] << ": ", *g, s0) << "\n";

                    ss.s.emplace_back(move(s0));
                    lit.acceptToken(t);

                    sp.s.emplace_back(termnode(t, *pt));

                    nextTok(*g, lit, ss);
                    break;
                } else {
                    bool r = reduce(ss, sp, lit, t.type, *pt);
                    if (!r) {
                        if (ti < len(lit.tok()) - 1) {
                            if (debug_pr)
                                std::cout << "Retry with same token of type " << g->ts[lit.tok()[ti + 1].type]
                                          << endl;

                            continue;
                        }
                        auto &t1 = lit.tok()[0];
                        auto hint = pt->error_handler->onNoShiftReduce(g, ss, sp, t1);
                        if (hint.type == ParseErrorHandler::Hint::Uncorrectable)
                            throw SyntaxError();
                        throw SyntaxError(
                                "Cannot shift or reduce : unexpected terminal {} = `{}` at {}"_fmt(g->ts[t1.type],
                                                                                                   t1.short_str(),
                                                                                                   t1.loc.beg));
                        //TODO: use hint instead of throwing exception
                    }
                    if (debug_pr)
                        printstate(std::cout << "Reduce by " << g->ts[t.type] << ": ", *g, ss.s.back()) << "\n";

                    lit.acceptToken(t);

                    if(sp.s.back()->type == ParseNode::Final) {
                        state = Paused;
                        sp.s.back()->type = ParseNode::FinalUsed;
                        return ParseTree(sp.s.back().get());
                    }
                    break;
                }
            }
resume:;
        }
        if (!reduce(ss, sp, lit, 0, *pt))
            throw SyntaxError("Unexpected end of file");
    } catch (SyntaxError &e) {
        if (e.stack_info.empty()) {
            e.stack_info = prstack(*g, ss, sp);
        }
        throw move(e);
    }
    Assert(ss.s.size() == 2);
    Assert(sp.s.size() == 1);
    state = AtEnd;
    if(sp.s[0]->type == ParseNode::FinalUsed)
        return ParseTree();
    return ParseTree(sp.s[0].get());
}

void GrammarState::error(const string & err) {
	cerr << "Error at line "<< lex.cpos().line<<':'<< lex.cpos().col<<" : " << err << "\n";
	_err.emplace_back(lex.cpos(), err);
}

void GrammarState::addLexerRule(const string & term, const string & rhs, bool tok, bool to_begin) {
	if (debug_pr)
		std::cout << "!!! Add lexer rule : " << term << " <- " << rhs << "\n";
	int nterms = ts.size();
	int n = tok ? ts[term] : 0;
	lex.addPEGRule(term, rhs, n, to_begin);
	if (n >= nterms) // Если добавился новый терминал
		for (auto& action : on_new_t_actions)
			action(this, term, n);
}

int GrammarState::addRule(const string & lhs, const vector<vector<string>>& rhs, SemanticAction act, int id, unsigned lpr, unsigned rpr) {
	if (debug_pr) {
		std::cout << "!!! Add rule  : " << lhs << " = ";
		for (auto&x : rhs) {
			if (x.empty())std::cout << " <ERROR: empty element>";
			else{
				std::cout << " " << x[0];
				for(int i=1; i<(int)x.size(); i++)
					std::cout << "+" << x[i];
			}
		}
		std::cout << "\n";
	}
	int ntnum = nts.size();
	CFGRule rule;
	rule.A = nts[lhs];
	rule.action = std::move(act);
	rule.lpr = (int)lpr; bool haslpr = rule.lpr != -1;
	rule.rpr = (int)rpr; bool hasrpr = rule.rpr != -1;
	if(rule.action && (haslpr || hasrpr))
		throw GrammarError("semantic actions not supported for rules with priorities");
	for (auto &y : rhs) {
		if (y.size() > 1) {
			string nm;
			bool nc = false;
			vector<pair<bool, string>> yy(y.size());
			for (int i = 0; i < (int)y.size(); i++) {
				if(i)nm += " + ";
				nm += y[i];
				if (y[i][0] == '\'' || y[i][0] == '"') {
					yy[i].first = false;
					yy[i].second = y[i].substr(1, y[i].size() - 2);
					if (ts.num(y[i]) < 0) {
						lex.addCToken(ts[y[i]], yy[i].second);
					}
				} else {
					yy[i].first = true;
					nc = true;
					if (ts.num(y[i]) >= 0)yy[i].second = y[i];
					else throw GrammarError("terminal `"+y[i]+"` not declared");
				}
			}
			lex.declareCompToken(yy, ts[nm]); 
			rule.rhs.push_back(RuleElem{ ts[nm],false,true,nc });
		} else if (y.empty()) continue;
		else {
			auto &x = y[0];
			if (x[0] == '\'') {
				if (ts.num(x) < 0) {
					lex.addCToken(ts[x], x.substr(1, x.size() - 2));
				}
				rule.rhs.push_back(RuleElem{ ts[x],true,true,false });                      // Константный терминал
			} else if (ts.num(x) >= 0)rule.rhs.push_back(RuleElem{ ts[x],false,true,true }); // Неконстантный терминал
			else {
				rule.rhs.push_back(RuleElem{ nts[x],false,false,true });                     // Нетерминал
			}
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
    if (rule.rhs.empty())
        throw GrammarError("empty rule right part: {} -> <empty>"_fmt(lhs));
	if (rule.rhs[0].term) {
		tf.checkSize(rule.A);
		auto &tmap = tFirstMap[rule.rhs[0].num];
		tFirstMap[rule.rhs[0].num].add(rule.A);
		tFirstMap[rule.rhs[0].num] |= tf.ifst[rule.A];
		if(!rule.rhs[0].cterm)
			tf.addRuleBeg_t(lex.pos(), rule.A, lex.internalNum(rule.rhs[0].num));
	} else tf.addRuleBeg(lex.pos(), rule.A, rule.rhs[0].num, len(rule.rhs));

	bool can_have_lpr = (!rule.rhs[0].term && rule.rhs[0].num == rule.A);
	bool can_have_rpr = (!rule.rhs.back().term && rule.rhs.back().num == rule.A);

	if (haslpr && rpr == lpr && (can_have_lpr != can_have_rpr)) { // Допустим лишь один приоритет, но заданы оба одинаковые, в этом случае удаляем лишний приоритет
		if (!can_have_lpr)rule.lpr = -1; // Если не должно быть левого приоритета
		else rule.rpr = -1;              // Если не должно быть правого приоритета
	} else {
		if (haslpr && !can_have_lpr)
			throw GrammarError("Left priority can be specified only for rules with right part starting from nonterminal from left side (A -> A B1 ... Bn)");

		if (hasrpr && !can_have_rpr)
			throw GrammarError("Right priority can be specified only for rules with right part ending by nonterminal from left side (A -> B1 ... Bn A)");
	}
	rule.ext_id = id;
	int num = (int)rules.size();
	rules.emplace_back(move(rule));
	if (!on_new_nt_actions.empty())
		for (int i = ntnum, end = nts.size(); i < end; i++)
			for (auto& action : on_new_nt_actions)
				action(this, nts[i], i);
	tf.checkSize((int)nts.size() - 1);

	return num - 1;
}

bool GrammarState::addRule(const CFGRule & rule) {
	NTTreeNode *curr = &root;
	int pos = 0;
	int ntnum = nts.size();
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

	if (rule.rhs[0].term) {
		tf.checkSize(rule.A);
		auto &tmap = tFirstMap[rule.rhs[0].num];
		tFirstMap[rule.rhs[0].num].add(rule.A);
		tFirstMap[rule.rhs[0].num] |= tf.ifst[rule.A];
	} else tf.addRuleBeg(lex.pos(), rule.A, rule.rhs[0].num, len(rule.rhs));

	rules.emplace_back(rule);
	if (!on_new_nt_actions.empty())
		for (int i = ntnum, end = nts.size(); i < end; i++)
			for (auto& action : on_new_nt_actions)
				action(this, nts[i], i);
	tf.checkSize((int)nts.size() - 1);

	return true;
}

bool GrammarState::addLexerRule(const ParseNode * tokenDef, bool tok, bool to_begin) {
	if (tokenDef->ch.size() != 2) {
		error("token definition must have 2 nodes");
		return false;
	}
	if (!tokenDef->ch[0]->isTerminal() || !tokenDef->ch[1]->isTerminal()) {
		error("token definition cannot contain nonterminals");
		return false;
	}
	addLexerRule(tokenDef->ch[0]->term, (tokenDef->ch[1]->term), tok, to_begin);
	return true;
}

ParseErrorHandler::Hint ParseErrorHandler::onRRConflict(GrammarState* g, const SStack& ss, const PStack& sp, int term, int nt1, int nt2, int depth, int place) {
	throw RRConflict("at {} RR-conflict({}) : 2 different ways to reduce by {}: NT = {} or {}"_fmt(g->lex.cpos(), place, g->ts[term], g->nts[nt1], g->nts[nt2]), prstack(*g, ss, sp, depth));
	return Hint();
}

ParseErrorHandler::Hint ParseErrorHandler::onNoShiftReduce(GrammarState* g, const SStack &ss, const PStack &sp, const Token& tok) {
	throw SyntaxError("Cannot shift or reduce : unexpected terminal {} = `{}` at {}"_fmt(g->ts[tok.type], tok.short_str(), tok.loc.beg.str()), prstack(*g, ss, sp));
	return Hint();
}
/**
 * Раскрывает квазицитату с заданными поддеревьями разбора
 * @param nt -- Нетерминал, который определяет тип квазицитаты q
 * @param q -- квазицитата
 * @param args -- список деревьев разбора, подставляемых в квазицитату
 * @param qid -- идентификатор правила, которым помечаются листья, на место которых подставляются поддеревья
 * */
ParseNode* ParseContext::quasiquote(const string &nt, const string& q, const std::initializer_list<ParseNode*>& args, int qid) {
	if (qid < 0)qid = _qid;
    if (!g->nts.has(nt)) throw GrammarError("Invalid quasiquote type `{}`: no such nonterminal in grammar"_fmt(nt));
	if (qid < 0)throw GrammarError("quasiquote argument rule id not set");
	bool qprev = _qq;
    _qq = true;
    try{
    	ParseTree t = parse(*this, q, nt);
        _qq = qprev;
        auto pos = args.begin();
        return replace_trees_rec(t.root.get(), pos, args.end(), qid);
    } catch (Exception &e) {
        e.prepend_msg("In quasiquote `{}`: "_fmt(q));
        throw e;
    }
}

/**
 * Раскрывает квазицитату с заданными поддеревьями разбора
 * @param nt -- Нетерминал, который определяет тип квазицитаты q
 * @param q -- квазицитата
 * @param args -- список деревьев разбора, подставляемых в квазицитату
 * @param qid -- идентификатор правила, которым помечаются листья, на место которых подставляются поддеревья
 * */
ParseNode* ParseContext::quasiquote(const string& nt, const string& q, const std::vector<ParseNode*>& args, int qid) {
	if (qid < 0)qid = _qid;
	if (!g->nts.has(nt)) throw GrammarError("Invalid quasiquote type `{}`: no such nonterminal in grammar"_fmt(nt));
	if (qid < 0)throw GrammarError("quasiquote argument rule id not set");
	try {
        ParseTree t = parse(*this, q, nt);
        auto pos = args.begin();
    	return replace_trees_rec(t.root.get(), pos, args.end(), qid);
    } catch (Exception &e){
        e.prepend_msg("In quasiquote `{}`: "_fmt(q));
        throw e;
    }
}

ParseNode* ParseContext::quasiquote(const string& nt, const string& q, const std::vector<ParseNodePtr>& args, int qid) {
    if (qid < 0)qid = _qid;
    if (!g->nts.has(nt)) throw GrammarError("Invalid quasiquote type `{}`: no such nonterminal in grammar"_fmt(nt));
    if (qid < 0)throw GrammarError("quasiquote argument rule id not set");
    try {
        ParseTree t = parse(*this, q, nt);
        auto pos = args.begin();
        return replace_trees_rec(t.root.get(), pos, args.end(), qid);
    } catch (Exception &e){
        e.prepend_msg("In quasiquote `{}`: "_fmt(q));
        throw e;
    }
}
