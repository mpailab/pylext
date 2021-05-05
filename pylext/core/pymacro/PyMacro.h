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
	unordered_map<string, int> nums;
	string uniq_name(const string& start) {
		int n = nums[start]++;
		return start + '_' + to_string(n);
	}
};

/// Контекст парсера для системы питоновских макрорасширений
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
    map<vector<vector<string>>, string, VecCmp> ntmap;
    PyMacroModule pymodule;
};

void init_python_grammar(PythonParseContext*px, bool read_by_stmt=true, const string& syntax_def="");
ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);

/////////////////////////////////////////////////////////////////////////////////////////
/// Обёртки для использования в Python

void del_python_context(PythonParseContext*);
PythonParseContext* create_python_context(bool read_by_stmt, const string & syntax_def);

std::string ast_to_text(ParseContext* pcontext, ParseNode *pn);

int add_lexer_rule(PythonParseContext *px, const string&nm, const string&rhs);
int add_token(PythonParseContext *px, const string& nm, const string& tokdef);

ParserState* new_parser_state(ParseContext* px, const string& text, const string& start);
ParseNode* continue_parse(ParserState* state);
bool at_end(ParserState *state);

extern "C" DLL_EXPORT void del_parser_state(ParserState* state);

extern "C" DLL_EXPORT int set_cpp_debug(int dbg);
