#pragma once
#include <string>
#include "Parser.h"

using namespace std;

struct PyMacro
{
	string name;
	int rule_id;
};

struct PySyntax
{
	string name;
	int rule_id;
};

struct PyMacroModule
{
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

string apply (string text);
void loadFile(const string &filename);

int pass_arg(int x);
int pass_arg_except(int x);

void init_python_grammar(PythonParseContext* px, bool read_by_stmt, const string &syntax_file);
ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);
bool equal_subtrees(ParseNode* x, ParseNode* y);
char* ast_to_text(ParseContext* px, ParseNode *pn);