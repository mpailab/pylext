#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <iomanip>
#include <chrono>  // for high_resolution_clock
#include <utility>
#include "Parser.h"
using namespace std;

int addRule(GrammarState& gr, const string& s, SemanticAction act = SemanticAction(), int id = -1, int lpr=-1, int rpr=-1);
int addRule(GrammarState& gr, const string& s, int id, int lpr=-1, int rpr=-1);
string loadfile(const string& fn);

std::string read_whole_file(const std::string& fn);

struct Timer {
	decltype(std::chrono::high_resolution_clock::now()) _t0;
	bool _started = false;
	string name;
	explicit Timer(string nm = "") :name(std::move(nm)) {}
	void start() {
		_t0 = std::chrono::high_resolution_clock::now();
		_started = true;
	}
	double stop() {
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
    Timer(const Timer&) = delete;
	Timer& operator =(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator = (Timer&&) = delete;
    ~Timer() {
		if (_started && !name.empty()) stop_pr();
	}
};

enum SynType {
	Or = 1,
	Maybe,
	Concat,
	Many,
	SynTypeLast=Many
};
template<class T>
vector<T>& operator +=(vector<T>& x, const vector<T>& y) {
	x.insert(x.end(), y.begin(), y.end());
	return x;
}
template<class T>
vector<T> operator +(vector<T> x, const vector<T>& y) {
	return x += y;
}
vector<vector<string>> getVariants(ParseNode* n);

void init_base_grammar(GrammarState& st, GrammarState* target);

/// f(f(x1,...,xn),y1,..,ym) -> f(x1,...,xn,y1,...,ym)
void flatten(ParseContext&, ParseNodePtr& n);
void flatten_p(ParseContext&, ParseNodePtr& n, int pos);
void flatten_check(ParseContext&, ParseNodePtr& n);
