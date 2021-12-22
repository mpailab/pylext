
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "Parser.h":

    cdef cppclass GrammarState:
        int ruleId(
            const string& lhs, 
            const vector[string]& rhs) const
        int addRule(
            const string &lhs, 
            const vector[string] &rhs, 
            int id, 
            int lpr, 
            int rpr)
        string sprint_rules() const

    cdef cppclass CParseContext "ParseContext":
        GrammarState& grammar()

    cdef cppclass CParseNode "ParseNode":
        int nt
        int refs
        int rule
        string term
        vector[CParseNode*] ch
        bool isTerminal() const

    cdef cppclass ParseNodePtr:
        CParseNode* get()

    cdef cppclass ParserState:
        ParserState(
            CParseContext *px, 
            string txt, 
            const string &start) except +
        ParseTree parse_next() except +

    cdef cppclass ParseTree:
        ParseNodePtr root

    bool equal_subtrees(CParseNode* x, CParseNode* y)
        
cdef extern from "GrammarUtils.h":

    int addRule(
        GrammarState& gr, 
        const string& s, 
        int id, 
        int lpr, 
        int rpr) except +

cdef extern from "PyMacro.h":

    cdef cppclass PythonParseContext(CParseContext):
        PythonParseContext()

    PythonParseContext* create_python_context(
        bool read_by_stmt, 
        const string& syntax_def) except +

    void del_python_context(PythonParseContext* px)

    string c_ast_to_text "ast_to_text" (
        CParseContext *px, 
        CParseNode *pn) except +

    int add_token(
        PythonParseContext *px, 
        const string& nm, 
        const string& tokdef) except +

    int add_lexer_rule(
        PythonParseContext *px, 
        const string& nm, 
        const string& rhs) except +

    CParseNode* c_quasiquote "quasiquote"(
        CParseContext*px, 
        const string& nt, 
        const vector[string]& parts,
        const vector[CParseNode*]& subtrees) except +

    int set_cpp_debug(int dbg)
