#pragma once
#include <unordered_map>
#include <any>
#include "Parser.h"
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
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

void init_python_grammar(GrammarState& g, bool read_by_stmt=true);
ParseNodePtr quasiquote(ParseContext& px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);

/////////////////////////////////////////////////////////////////////////////////////////
/// Обёртки для простого экспорта из dll
extern "C" DLL_EXPORT void* c_quasiquote(void* px, char* nt, int n, char** data, void** pn);

extern "C" DLL_EXPORT void* new_python_context(int by_stmt);
extern "C" DLL_EXPORT void del_python_context(void*);


extern "C" DLL_EXPORT void inc_pn_num_refs(void *pn);
extern "C" DLL_EXPORT void dec_pn_num_refs(void *pn);

extern "C" DLL_EXPORT int get_pn_num_children(void* pn);
extern "C" DLL_EXPORT void* get_pn_child(void* pn, int i);
extern "C" DLL_EXPORT void set_pn_child(void* pn, int i, void* ch);

extern "C" DLL_EXPORT int get_pn_rule(void* pn);
extern "C" DLL_EXPORT int pn_equal(void* pn1, void* pn2);


extern "C" DLL_EXPORT int add_rule(void* px, char* lhs, char *rhs);

extern "C" DLL_EXPORT void* new_parser_state(void* px, const char* text, const char *start);
extern "C" DLL_EXPORT void* continue_parse(void* state);
extern "C" DLL_EXPORT void del_parser_state(void* state);



