#pragma once
#include <string>

using namespace std;

string apply (string text);

void* c_quasiquote(void* px, char* nt, int n, void* data, void* pn);

void* new_python_context(int by_stmt);
void del_python_context(void*);


void inc_pn_num_refs(void *pn);
void dec_pn_num_refs(void *pn);

int get_pn_num_children(void* pn);
void* get_pn_child(void* pn, int i);
void set_pn_child(void* pn, int i, void* ch);

int get_pn_rule(void* pn);
int pn_equal(void* pn1, void* pn2);


int add_rule(void* px, char* lhs, char *rhs);

void* new_parser_state(void* px, const char* text, const char *start);
void* continue_parse(void* state);
void del_parser_state(void* state);