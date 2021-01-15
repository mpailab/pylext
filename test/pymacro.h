#pragma once
#include <unordered_map>
#include <any>
#include "Parser.h"
#ifdef MSC_VER
#define DLL_EXPORT __declspec(dllexport)
#else
#ifdef _WIN32
#define DLL_EXPORT __attribute__((dllexport))
#else
#define DLL_EXPORT
#endif
#endif

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
ParseNodePtr quasiquote(ParseContext& px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);




