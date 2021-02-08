
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "Parser.h":
    cdef cppclass ParseNode

cdef extern from "apply.h":
    string c_apply "apply" (string text)
    void c_loadFile "loadFile" (const string &filename) except +
    int c_pass_arg "pass_arg" (int x)
    int c_pass_arg_except "pass_arg_except" (int x) except +
    void* c_c_quasiquote "c_quasiquote" (void* px, const string& nt, const vector[string]& data, const vector[ParseNode*]& subtrees) except +
    void* c_new_python_context "new_python_context" (int by_stmt, const string& syntax_file)
    void c_del_python_context "del_python_context" (void*)
    void c_inc_pn_num_refs "inc_pn_num_refs" (void* pn)
    void c_dec_pn_num_refs "dec_pn_num_refs" (void* pn)
    int c_get_pn_num_children "get_pn_num_children" (void* pn)
    void* c_get_pn_child "get_pn_child" (void* pn, int i)
    int c_set_pn_child "set_pn_child" (void* pn, int i, void* ch)
    int c_get_pn_rule "get_pn_rule" (void* pn)
    int c_pn_equal "pn_equal" (void* pn1, void* pn2)
    int c_add_rule "add_rule" (void* px, const string& lhs, const string& rhs)
    void* c_new_parser_state "new_parser_state" (void* px, const string& text, const string& start)
    void* c_continue_parse "continue_parse" (void* state)
    void c_del_parser_state "del_parser_state" (void* state)
    char* c_ast_to_text "ast_to_text" (void* pcontext, void* pn)