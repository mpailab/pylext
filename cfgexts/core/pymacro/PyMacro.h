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


class PythonParseContext: public ParseContext{
    struct VecCmp {
        template<class T>
        bool operator()(const T& x, const T& y)const { return x<y; }

        template<class T>
        bool operator()(const vector<T>&x, const vector<T>&y) const {
            return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), *this);
        }
    };
public:
    PythonParseContext() = default;
    explicit PythonParseContext(GrammarState* g): ParseContext(g){}
    map<vector<vector<vector<string>>>, string, VecCmp> ntmap;
    PyMacroModule pymodule;
};

void init_python_grammar(PythonParseContext*px, bool read_by_stmt=true, const string& syntax_def="");
ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);

/////////////////////////////////////////////////////////////////////////////////////////
/// Обёртки для простого экспорта из dll
extern "C" DLL_EXPORT char* get_last_error();

//extern "C" DLL_EXPORT void* c_quasiquote(void* px, char* nt, int n, char** data, void** pn);

//PythonParseContext* new_python_context(int by_stmt);
void del_python_context(PythonParseContext*);
PythonParseContext* create_python_context(bool read_by_stmt, const string & syntax_def);


extern "C" DLL_EXPORT void inc_pn_num_refs(void *pn);
extern "C" DLL_EXPORT void dec_pn_num_refs(void *pn);

std::string ast_to_text(ParseContext* pcontext, ParseNode *pn);
bool equal_subtrees(ParseNode* x, ParseNode* y);

extern "C" DLL_EXPORT int get_pn_num_children(void* pn);
extern "C" DLL_EXPORT void* get_pn_child(void* pn, int i);
extern "C" DLL_EXPORT int set_pn_child(void* pn, int i, void* ch);

extern "C" DLL_EXPORT int get_pn_ntnum(void* pn);
extern "C" DLL_EXPORT int get_pn_rule(void* pn);
extern "C" DLL_EXPORT const char* get_terminal_str(void* pn);
extern "C" DLL_EXPORT int pn_equal(void* pn1, void* pn2);


int add_lexer_rule(PythonParseContext *px, const string&nm, const string&rhs);
int add_token(PythonParseContext *px, const string& nm, const string& tokdef);
//extern "C" DLL_EXPORT int add_token(void* pxv, const char* nm, const char* tokdef);
//extern "C" DLL_EXPORT int add_lexer_rule(void* pxv, const char* nm, const char* rhs);

extern "C" DLL_EXPORT int add_rule(void* px, char* lhs, char *rhs, int lpr, int rpr);

ParserState* new_parser_state(ParseContext* px, const string& text, const string& start);
ParseNode* continue_parse(ParserState* state);
bool at_end(ParserState *state);

extern "C" DLL_EXPORT void del_parser_state(ParserState* state);

extern "C" DLL_EXPORT int set_cpp_debug(int dbg);
