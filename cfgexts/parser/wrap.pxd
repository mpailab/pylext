
from libcpp.string cimport string
from cpython.ref cimport PyObject

cdef extern from "apply.h":
    string c_apply "apply" (string text)
    void c_loadFile "loadFile" (const string &filename) except +
    int c_pass_arg "pass_arg" (int x)
    int c_pass_arg_except "pass_arg_except" (int x) except +
    PyObject* c_quasiquote "c_quasiquote" (PyObject* px, char* nt, int n, PyObject* data, PyObject* pn)
    PyObject* c_new_python_context "new_python_context" (int by_stmt, string syntax_file)
    void c_del_python_context "del_python_context" (PyObject*)
    void c_inc_pn_num_refs "inc_pn_num_refs" (void *pn)
    void c_dec_pn_num_refs "dec_pn_num_refs" (void *pn)
    int c_get_pn_num_children "get_pn_num_children" (PyObject* pn)
    PyObject* c_get_pn_child "get_pn_child" (PyObject* pn, int i)
    void c_set_pn_child "set_pn_child" (PyObject* pn, int i, PyObject* ch)
    int c_get_pn_rule "get_pn_rule" (PyObject* pn)
    int c_pn_equal "pn_equal" (PyObject* pn1, PyObject* pn2)
    int c_add_rule "add_rule" (PyObject* px, char* lhs, char *rhs)
    PyObject* c_new_parser_state "new_parser_state" (PyObject* px, const char* text, const char *start)
    PyObject* c_continue_parse "continue_parse" (PyObject* state)
    void c_del_parser_state "del_parser_state" (PyObject* state)