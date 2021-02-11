
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "Parser.h":
    cdef cppclass GrammarState
    cdef cppclass ParseContext:
        GrammarState& grammar()
    cdef cppclass ParseNode:
        int rule
        int refs
        vector[ParseNode*] ch
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

    void init_python_grammar(PythonParseContext* px, bool read_by_stmt, const string &syntax_file) except +
    ParseNode* quasiquote(ParseContext* px, const string& nt, const vector[string]& parts, const vector[ParseNode*]& subtrees) except +
    bool equal_subtrees(ParseNode* x, ParseNode* y)
    char* c_ast_to_text "ast_to_text" (ParseContext* px, ParseNode *pn) except +

cdef extern from "base.h":
    int addRule(GrammarState& gr, const string& s) except +