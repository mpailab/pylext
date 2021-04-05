
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "Parser.h":
    cdef cppclass GrammarState
    cdef cppclass CParseContext "ParseContext":
        GrammarState& grammar()
    cdef cppclass CParseNode "ParseNode":
        int rule, nt
        int refs
        string term
        vector[CParseNode*] ch
        int rule_nt()const
        bool isTerminal()const

    cdef cppclass ParseNodePtr:
        CParseNode* get()
    cdef cppclass ParserState:
        ParserState(CParseContext *px, string txt, const string &start) except +
        ParseTree parse_next() except +
    cdef cppclass ParseTree:
        ParseNodePtr root
    cdef cppclass PythonParseContext(CParseContext):
        PythonParseContext()
    cdef cppclass SemanticAction

# cdef extern from "apply.h":
#     string c_apply "apply" (string text)
#     void c_loadFile "loadFile" (const string &filename) except +
#     int c_pass_arg "pass_arg" (int x)
#     int c_pass_arg_except "pass_arg_except" (int x) except +

    # void init_python_grammar(PythonParseContext* px, bool read_by_stmt, const string &syntax_file) except +
    # ParseNode* quasiquote(ParseContext* px, const string& nt, const vector[string]& parts, const vector[ParseNode*]& subtrees) except +
    # bool equal_subtrees(ParseNode* x, ParseNode* y)
    # char* c_ast_to_text "ast_to_text" (ParseContext* px, ParseNode *pn) except +

cdef extern from "GrammarUtils.h":
    int addRule(GrammarState& gr, const string& s, int id, int lpr, int rpr) except +

cdef extern from "PyMacro.h":
    PythonParseContext* create_python_context(bool read_by_stmt, const string & syntax_def) except +
    void del_python_context(PythonParseContext* px)

    bool equal_subtrees(CParseNode*x, CParseNode*y)
    string c_ast_to_text "ast_to_text"(CParseContext *px, CParseNode *pn) except +

    int add_token(PythonParseContext *px, const string& nm, const string& tokdef) except +
    int add_lexer_rule(PythonParseContext *px, const string& nm, const string& rhs) except +

    const char *get_terminal_str(CParseNode *pn)
    CParseNode* c_quasiquote "quasiquote"(CParseContext*px, const string& nt, const vector[string]& parts,
                         const vector[CParseNode*]& subtrees) except +

    ParserState* new_parser_state(CParseContext *px, const string& text, const string& start)
    CParseNode* continue_parse(ParserState *st)
    bool at_end(ParserState* st)

    int set_cpp_debug(int dbg)
