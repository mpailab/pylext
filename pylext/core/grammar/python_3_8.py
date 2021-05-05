python_grammar_str = '''
%token: comment /= `'#' [^\\n]*`;

#################################

%indent: INDENT;
%dedent: DEDENT;
# %check_indent: INDENTED;
%eol: EOL;

# %syntax: new_syntax -> new_syntax_expr ';';

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#%syntax: if_stmt     -> if_stmt_noelse ['else' ':' suite];

%token: ident = `[_a-zA-Z][_a-zA-Z0-9]*`;

%token: shortstring     =  `'\\'' ('\\\\'[^] / [^\\\\\\n'])* '\\'' / '"' ('\\\\'[^] / [^\\\\\\n"])* '"'`;
%token: stringliteral   =  `(([fF][rR]/[rR][fF]/[rufRUF])?(longstring / shortstring))+`;
%token: longstring      =  `'\\'\\'\\'' ('\\\\'[^] / !('\\'\\'\\'')[^\\\\])* '\\'\\'\\'' / '"""' ('\\\\'[^] / !('"""')[^\\\\])* '"""'`;

%token: bytesliteral    =  `([bB][rR]? / [rR][bB]) (longstring / shortstring)`;
# %token: sstring       = `shortstring`;
# %syntax: text -> stringliteral | bytesliteral;

%token: integer      =  `bininteger / octinteger / hexinteger / decinteger`;
%pexpr: decinteger   =  `[1-9] ('_'?[0-9])* / '0'+ ('_' '0')*`;
%pexpr: bininteger   =  `'0'[bB] ('_'?[01])+`;
%pexpr: octinteger   =  `'0'[oO] ('_'?[0-7])+`;
%pexpr: hexinteger   =  `'0'[xX] ('_'?[0-9a-fA-F])+`;

%token: floatnumber   =  `pointfloat / exponentfloat`;
%pexpr: digitpart     =  `[0-9] ('_'?[0-9])*`;
%pexpr: pointfloat    =  `digitpart? '.' digitpart / digitpart '.'!('.')`;
%pexpr: exponentfloat =  `(digitpart / pointfloat) [Ee][+\\-]? digitpart`;


%token: imagnumber = `(floatnumber / digitpart)[jJ]`;

# %syntax: text -> root_stmts;
# %syntax: root_stmts -> [root_stmts] root_stmt;
%syntax: root_stmt -> stmt;

%syntax: decorator -> '@' dotted_ident ['(' [arglist] ')'] EOL;
# %syntax: decorators -> decorator [decorators];
%syntax: decorated -> {decorator} classdef;
%syntax: decorated -> {decorator} funcdef;
%syntax: decorated -> {decorator} async_funcdef;

%syntax: async_funcdef -> 'async' funcdef;
%syntax: funcdef -> 'def' ident parameters '->' test ':' [longstring] func_body_suite;
%syntax: funcdef -> 'def' ident parameters ':' [longstring] func_body_suite;

%syntax: parameters -> '(' [typedargslist] ')';

# The following definition for typedarglist is equivalent to this set of rules:
#
#     arguments = argument (',' [TYPE_COMMENT] argument)*
#     argument = tfpdef ['=' test]
#     kwargs = '**' tfpdef [','] [TYPE_COMMENT]
#     args = '*' [tfpdef]
#     kwonly_kwargs = (',' [TYPE_COMMENT] argument)* (TYPE_COMMENT | [',' [TYPE_COMMENT] [kwargs]])
#     args_kwonly_kwargs = args kwonly_kwargs | kwargs
#     poskeyword_args_kwonly_kwargs = arguments ( TYPE_COMMENT | [',' [TYPE_COMMENT] [args_kwonly_kwargs]])
#     typedargslist_no_posonly  = poskeyword_args_kwonly_kwargs | args_kwonly_kwargs
#     typedarglist = (arguments ',' [TYPE_COMMENT] '/' [',' [[TYPE_COMMENT] typedargslist_no_posonly]])|(typedargslist_no_posonly)"
#
# It needs to be fully expanded to allow our LL(1) parser to work on it.


%syntax: arguments -> argument | arguments ',' [longstring] argument;
%syntax: argument -> tfpdef ['=' test];
%syntax: kwargs -> '**' tfpdef [','] [longstring];
%syntax: args -> '*' [tfpdef];
%syntax: kwonly_kwargs -> ',' [longstring] argument [kwonly_kwargs] | longstring;
%syntax: kwonly_kwargs -> ',' [longstring] [kwargs];
%syntax: args_kwonly_kwargs -> args [kwonly_kwargs] | kwargs;
%syntax: poskeyword_args_kwonly_kwargs -> arguments [longstring];
%syntax: poskeyword_args_kwonly_kwargs -> arguments ',' [longstring] [args_kwonly_kwargs];
%syntax: typedargslist_no_posonly -> poskeyword_args_kwonly_kwargs | args_kwonly_kwargs;
%syntax: typedargslist -> (arguments ',' [longstring] '/' [',' [[longstring] typedargslist_no_posonly]]) | typedargslist_no_posonly;

%syntax: tfpdef -> ident [':' test];

 ########################################################################

%syntax: arguments1    -> [arguments1 ','] argument1;
%syntax: argument1     -> ident ['=' test];
%syntax: kwargs1       -> '**' ident [','];
%syntax: args1         -> '*' [ident];
%syntax: kwonly_kwargs1 -> ',' [kwargs1];
%syntax: kwonly_kwargs1 -> ',' argument1 kwonly_kwargs1;
%syntax: args_kwonly_kwargs1 -> args1 [kwonly_kwargs1];
%syntax: args_kwonly_kwargs1 -> kwargs1;
%syntax: poskeyword_args_kwonly_kwargs1 -> arguments1 [',' [args_kwonly_kwargs1]];
%syntax: vararglist_no_posonly -> poskeyword_args_kwonly_kwargs1 | args_kwonly_kwargs1;
%syntax: varargslist -> arguments1 ',' '/' [',' [vararglist_no_posonly]] | vararglist_no_posonly;

 ########################################################################

%token: augassign = `'+=' / '-=' / '*=' / '@=' / '/=' / '%=' / '&=' / '|=' / '^=' / '<<=' / '>>=' / '**=' / '//='`;

%syntax: stmt -> simple_stmt | compound_stmt;
%syntax: simple_stmt -> small_stmt ';' simple_stmt | small_stmt EOL;
%syntax: small_stmt  -> expr_stmt | del_stmt | pass_stmt | flow_stmt | global_stmt | nonlocal_stmt | assert_stmt
                      | 'from' ([dots] dotted_name | dots) 'import' ('*' | '(' import_as_names ')' | import_as_names)
                      | 'import' dotted_as_names;
%token: dots = `'.'+`;
%syntax: import_as_name -> ident ['as' ident];
%syntax: dotted_name -> ident ['.' dotted_name];
%syntax: dotted_as_name -> dotted_name ['as' ident];
%syntax: import_as_names -> import_as_name [',' import_as_names] | import_as_name ',';
%syntax: dotted_as_names -> dotted_as_name [',' dotted_as_names];

%syntax: assignlist -> '=' (yield_expr | testlist_star_expr) [longstring] | '=' (yield_expr | testlist_star_expr) assignlist;
%syntax: expr_stmt -> testlist_star_expr [annassign | augassign (yield_expr | testlist) | assignlist];
%syntax: annassign -> ':' test ['=' (yield_expr | testlist_star_expr)];

%syntax: testlist_star_expr -> (test | star_expr) [',' [testlist_star_expr]];

# For normal and annotated assignments, additional restrictions enforced by the interpreter
%syntax: del_stmt      -> 'del' exprlist;
%syntax: pass_stmt     -> 'pass';
%syntax: flow_stmt     -> 'break' | 'continue'
                        | 'return' [testlist_star_expr]
                        | 'raise' [test ['from' test]]
                        | yield_expr;

 # note below: the ('.' | '...') is necessary because '...' is tokenized as ELLIPSIS
%syntax: import_as_ident  -> ident ['as' ident];
%syntax: dotted_as_ident  -> dotted_ident ['as' ident];
%syntax: import_as_idents -> import_as_ident [',' [import_as_idents]];
%syntax: dotted_as_idents -> dotted_as_ident [',' dotted_as_idents];
%syntax: dotted_ident     -> [dotted_ident '.'] ident;
%syntax: global_stmt      -> 'global' ident | global_stmt ',' ident;
%syntax: nonlocal_stmt    -> 'nonlocal' ident | nonlocal_stmt ',' ident;
%syntax: assert_stmt      -> 'assert' test [',' test];

%syntax: compound_stmt   -> if_stmt | while_stmt | for_stmt | try_stmt | with_stmt | funcdef | classdef | decorated | async_stmt;
%syntax: async_stmt      -> 'async' (funcdef | with_stmt | for_stmt);
%syntax: if_stmt_noelse  -> 'if' identdexpr_test ':' suite | if_stmt_noelse 'elif' identdexpr_test ':' suite;
##!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
%syntax: if_stmt         -> if_stmt_noelse ['else' ':' suite];

%syntax: while_stmt      -> 'while' identdexpr_test ':' suite ['else' ':' suite];
%syntax: for_stmt        -> 'for' exprlist 'in' testlist ':' [longstring] suite ['else' ':' suite];
%syntax: try_stmt        -> 'try' ':' suite 'finally' ':' suite
                          | 'try' ':' suite excepts ['else' ':' suite] ['finally' ':' suite];
%syntax: excepts         -> except_clause ':' suite | excepts except_clause ':' suite;
%syntax: with_items      -> [with_items ','] with_item;
%syntax: with_stmt       -> 'with' with_items ':' [longstring] suite;
%syntax: with_item       -> test ['as' expr];

%syntax: except_clause   -> 'except' [test ['as' ident]];

# %syntax: indented_stmt   -> stmt; # | stmts1 stmt;

%syntax: suite           -> simple_stmt | EOL INDENT {stmt} DEDENT;
# %syntax: suite         -> INDENT {indented_stmt} DEDENT;

%syntax: identdexpr_test -> test [':=' test];
%syntax: test            -> or_test | or_test 'if' or_test 'else' test | lambdef;
%syntax: test_nocond     -> or_test | lambdef_nocond;
%syntax: lambdef         -> 'lambda' ':' test | 'lambda' varargslist ':' test;
%syntax: lambdef_nocond  -> 'lambda' ':' test_nocond | 'lambda' varargslist ':' test_nocond;
%syntax: or_test         -> and_test | or_test 'or' and_test;
%syntax: and_test        -> not_test | and_test 'and' not_test;
%syntax: not_test        -> 'not' not_test | comparison;

%syntax: comparison      -> expr | comparison comp_op expr;

# <> isn't actually a valid comparison operator in Python. It's here for the
# sake of a __future__ import described in PEP 401 (which really works :-)
%syntax: comp_op         -> '<'|'>'|'=='|'>='|'<='|'<>'|'!='|'in'|'not' 'in'|'is'|'is' 'not';
%syntax: star_expr       -> '*' expr;

%prefix(expr,100) : '+' | '-' | '~';
%infxr(expr,70) : '**';
%infxl(expr,60) : '*' | '@' | '/' | '%' | '//';
%infxl(expr,50) : '+' | '-';
%infxl(expr,40) : '<<' | '>>';
%infxl(expr,30) : '&';
%infxl(expr,20) : '^';
%infxl(expr,10) : '|';

%syntax: expr            -> atom_expr; # | expr ('*' | '@' | '/' | '%' | '//' | '+' | '-' | '**' | '<<' | '>>' | '&' | '|' | '^') expr | ('+' | '-' | '~') expr;
%syntax: atom_expr       -> ['await'] atom | atom_expr trailer;
%syntax: atom            -> '(' [yield_expr | testlist_comp] ')'
                          | '[' [testlist_comp] ']'
                          | '{' [dictorsetmaker] '}'
                          | ident | NUMBER | stringliteral | bytesliteral | '...' | 'None' | 'True' | 'False';
%syntax: identd_or_star  -> identdexpr_test | star_expr;
%syntax: identd_or_star_list -> [identd_or_star_list ','] identd_or_star;
%syntax: testlist_comp   -> identd_or_star comp_for |  identd_or_star_list [','];


%syntax: trailer        -> '(' [arglist] ')' | '[' subscriptlist ']' | '.' ident;
%syntax: subscriptlist  -> subscript [',' [subscriptlist]];
%syntax: subscript      -> test | [test] ':' [test] [':' [test]];
%syntax: exprlist       -> (expr | star_expr) [',' [exprlist]];
%syntax: testlist       -> test [',' [testlist]];
%syntax: ds1            -> test ':' test | '**' expr;
%syntax: ds2            -> ds1 | ds2 ',' ds1;
%syntax: dictmaker      -> ds2 | ds1 comp_for;
%syntax: ds4            -> test | star_expr;
%syntax: ds5            -> ds4 | ds5 ',' ds4;
%syntax: setmaker       -> ds5 | ds4 comp_for;
%syntax: dictorsetmaker -> dictmaker | setmaker;

%syntax: classdef -> 'class' ident ['(' [arglist] ')'] ':' suite;

%syntax: arglist -> argument2 [',' [arglist]];

%syntax: argument2 -> test
                    | test comp_for
                    | test ':=' test
                    | test '=' test
                    | '**' test
                    | '*' test;

%syntax: comp_iter      -> comp_for | comp_if;
%syntax: sync_comp_for  -> 'for' exprlist 'in' or_test [comp_iter];
%syntax: comp_for       -> ['async'] sync_comp_for;
%syntax: comp_if        -> 'if' test_nocond [comp_iter];

%syntax: yield_expr -> 'yield' ['from' test | testlist_star_expr];

# the longstring in suites is only parsed for funcdefs,
# but can't go elsewhere due to ambiguity
%syntax: func_body_suite -> simple_stmt
                          | EOL INDENT {stmt} DEDENT
                          | EOL longstring INDENT {stmt} DEDENT;

%syntax: NUMBER  -> floatnumber | integer | imagnumber;
'''
