#pragma once
#include <string>

using namespace std;

string apply (string text);
void loadFile(const string &filename);

int pass_arg(int x);
int pass_arg_except(int x);

void* c_quasiquote(void* px, char* nt, int n, char** data, void** pn);

void* new_python_context(int by_stmt, string syntax_file);
void del_python_context(void*);


void inc_pn_num_refs(void* pn);
void dec_pn_num_refs(void* pn);

int get_pn_num_children(void* pn);
void* get_pn_child(void* pn, int i);
int set_pn_child(void* pn, int i, void* ch);

int get_pn_rule(void* pn);
int pn_equal(void* pn1, void* pn2);


int add_rule(void* px, char* lhs, char *rhs);

void* new_parser_state(void* px, const char* text, const char *start);
void* continue_parse(void* state);
void del_parser_state(void* state);

char* ast_to_text(void* pcontext, void* pn);