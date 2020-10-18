#pragma once
#include <unordered_map>
#include <any>
#include "Parser.h"
using namespace std;

struct PyMacro {
	string name;
	int rule_id;
};
struct PySyntax {
	string name;
	int rule_id;
};

struct PyMacroModule {
	unordered_map<int, PyMacro> macros;
	unordered_map<int, PySyntax> syntax;
	//int num = 0;
	unordered_map<string, int> nums;
	string uniq_name(const string& start) {
		int n = nums[start]++;
		return start + '_' + to_string(n);
	}
};

void init_python_grammar(GrammarState& g);


