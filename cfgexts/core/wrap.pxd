
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "Parser.h":
    cdef cppclass GrammarState
    cdef cppclass ParseContext:
        GrammarState& grammar()
    cdef cppclass ParseNode:
        int rule, nt
        int refs
        string term
        vector[ParseNode*] ch
        int rule_nt()const
        bool isTerminal()const

    cdef cppclass ParseNodePtr:
        ParseNode* get()
    cdef cppclass ParserState:
        ParserState(ParseContext *px, string txt, const string &start) except +
        ParseTree parse_next() except +
    cdef cppclass ParseTree:
        ParseNodePtr root
    cdef cppclass PythonParseContext:
        PythonParseContext()
    cdef cppclass SemanticAction

cdef extern from "apply.h":
    string c_apply "apply" (string text)
    void c_loadFile "loadFile" (const string &filename) except +
    int c_pass_arg "pass_arg" (int x)
    int c_pass_arg_except "pass_arg_except" (int x) except +

    # void init_python_grammar(PythonParseContext* px, bool read_by_stmt, const string &syntax_file) except +
    # ParseNode* quasiquote(ParseContext* px, const string& nt, const vector[string]& parts, const vector[ParseNode*]& subtrees) except +
    # bool equal_subtrees(ParseNode* x, ParseNode* y)
    # char* c_ast_to_text "ast_to_text" (ParseContext* px, ParseNode *pn) except +

cdef extern from "GrammarUtils.h":
    int addRule(GrammarState& gr, const string& s, int id, int lpr, int rpr) except +

cdef extern from "PyMacro.h":
    PythonParseContext* create_python_context(bool read_by_stmt, const string & syntax_def) except +
    void del_python_context(PythonParseContext* px)

    bool equal_subtrees(ParseNode*x, ParseNode*y)
    char* ast_to_text(ParseContext *px, ParseNode *pn) except +

    int add_token(PythonParseContext *px, const string& nm, const string& tokdef) except +
    int add_lexer_rule(PythonParseContext *px, const string& nm, const string& rhs) except +

    const char *get_terminal_str(ParseNode *pn)
    ParseNode* quasiquote(ParseContext*px, const string& nt, const vector[string]& parts,
                         const vector[ParseNode*]& subtrees) except +

    ParserState* new_parser_state(ParseContext *px, const string& text, const string& start)
    ParseNode* continue_parse(ParserState *st)
    bool at_end(ParserState* st)

    int set_cpp_debug(int dbg)
