# cython: c_string_type=unicode, c_string_encoding=utf8

"""
Parsing python language extensions and transforming them to standard python grammar.
"""

# External imports
from libcpp.string cimport string
from libcpp.vector cimport vector

# Internal imports
from .parse cimport *
from .grammar.python_3_8 import python_grammar_str


class InvalidParseContext(Exception):
    def __init__(self):
        super().__init__("Request parse context after gimport finished")


cdef class ParseNode:
    """
    A wrapper class for C++ class ParseNode that represents node of parse tree.

    Attributes
    ----------
    _ptr : CParseNode*
        Pointer to C++ class ParseNode
    """

    cdef CParseNode* _ptr
    cdef object _ctx  # parse context

    def __init__(self):
        """Class constructor"""
        self._ptr = NULL
        self._ctx = None

    def __del__(self):
        """Python class destructor"""
        if self._ptr != NULL and self._ctx is not None and self._ctx.is_valid():
            self._ptr.refs -= 1
            self._ptr = NULL

    def __dealloc__(self):
        """C++ class destructor"""
        if self._ptr != NULL and self._ctx is not None and self._ctx.is_valid():
            self._ptr.refs -= 1
            self._ptr = NULL

    @staticmethod
    cdef ParseNode from_ptr(CParseNode *ptr, ctx):
        """
        Factory function to create ParseNode object from given CParseNode pointer.

        Parameters
        ----------
        ptr : CParseNode*
            Pointer to C++ class ParseNode

        Returns
        -------
        ParseNode
            Python object ParseNode.
        """
        assert ptr != NULL

        # Create new wrapper for given pointer
        cdef ParseNode wrapper = ParseNode.__new__(ParseNode)
        wrapper._ptr = ptr
        wrapper._ctx = ctx

        # Increase the number of references to given pointer
        ptr.refs += 1

        return wrapper

    cdef CParseNode* to_ptr(self):
        """Get CParseNode pointer of ParseNode object"""
        return self._ptr

    def children(self):
        """
        Get list of children of given node in parse tree.

        Returns
        -------
        list
            List of ParseNode objects.
        """
        return [ParseNode.from_ptr(x, self._ctx) for x in self._ptr.ch]

    cpdef size_t num_children(self):
        """
        Get number of children of given node in parse tree.

        Returns
        -------
        int
            Number of children of given node in parse tree.
        """
        return self._ptr.ch.size()

    cpdef ParseNode getitem(self, int i):
        """
        Get child of given node in parse tree. Internal part of __getitem__.

        Parameters
        ----------
        i : int
            Child number of given parse node.

        Returns
        -------
        ParseNode
            Child of given node with number i.
        """

        # Check out of range
        cdef size_t n = self._ptr.ch.size()
        if i < 0 or i >= n:
            raise ValueError(f"Parse node child index {i} out of range ({n})")

        return ParseNode.from_ptr(self._ptr.ch[i], self._ctx)

    def __getitem__(self, i : int):
        """
        Get child of given node in parse tree.

        Parameters
        ----------
        i : int
            Child number of given parse node.

        Returns
        -------
        ParseNode
            Child of given node with number i.
        """
        return self.getitem(i)

    cpdef void setitem(self, int i, ParseNode child):
        """
        Set child of given node in parse tree. Internal part of __setitem__.

        Parameters
        ----------
        i : int
            Child number of given parse node.
        child : ParseNode
            Parse node.
        """

        # Check out of range
        cdef size_t n = self._ptr.ch.size()
        if i < 0 or i >= n:
            raise ValueError(f"Parse node child index {i} out of range ({n})")

        self._ptr.ch[i] = child._ptr

    def __setitem__(self, i : int, child : ParseNode):
        """
        Set child of given node in parse tree.

        Parameters`
        ----------
        i : int
            Child number of given parse node.
        child : ParseNode
            Parse node.
        """
        self.setitem(i, child)

    cpdef int rule(self):
        """
        Get the syntax rule ID of given node in parse tree.

        Returns
        -------
        int
           ID of syntax rule by which given parse node were obtained.
        """
        return self._ptr.rule

    cpdef bool is_terminal(self):
        """
        Check is given node in parse tree terminal.

        Returns
        -------
        bool
           True if given parse node is terminal, and False otherwise.
        """
        return self._ptr.isTerminal()

    cpdef string str(self):
        """
        Get the string representation of given node in parse tree.

        Returns
        -------
        string
           String represented given parse node tree.
        
        Warnings
        --------
        Can be applied only to terminal parse node.
        """
        assert self._ptr.isTerminal()
        return self._ptr.term

    cpdef int ntnum(self):
        """
        Get the symbol (terminal or nonterminal) ID of given node in parse tree.

        Returns
        -------
        int
           ID of symbol corresponding to given parse node.
        """
        return self._ptr.nt

    cdef bool equal(self, ParseNode other):
        """
        Check is given node in parse tree equal to other node in parse tree.
        Internal part of __eq__.

        Parameters
        ----------
        other : ParseNode
            Parse node.

        Returns
        -------
        bool
           True if given parse node is equal to other parse node, and False otherwise.
        """
        return equal_subtrees(self._ptr, other._ptr) != 0

    def __eq__(self, other):
        """
        Check is given node in parse tree equal to other node in parse tree.

        Parameters
        ----------
        other : ParseNode
            Parse node.

        Returns
        -------
        bool
           True if given parse node is equal to other parse node, and False otherwise.
        """
        return self.equal(other)

    def __str__(self):
        """String representation of given node in parse tree"""
        return ast_to_text(self)

    def __repr__(self):
        """Official string representation of given node in parse tree"""
        return 'ParseNode:\n' + str(self)


__parse_context__ = None


def parse_context():
    """
    Get current parse context.

    Returns
    -------
    ParseContext
        Parse context of current module.

    Raises
    ------
    InvalidParseContext
        If function called not during pyg module import

    Note
    ----
        Do not store result of this function in class attributes
        and don't capture it in lambda functions.
        After module import is finished, this object will be destroyed.
    """
    global __parse_context__
    if __parse_context__ is None:
        raise InvalidParseContext()

    return __parse_context__


def parse_context_not_check():
    """
    Get current parse context without check; for internal purposes

    Returns
    -------
    ParseContext
        Parse context of current module.
    """
    global __parse_context__
    return __parse_context__


cdef class ParseContext:
    """
    A wrapper class for C++ class PythonParseContext that represents parsing context.

    Attributes
    ----------
    _ptr : PythonParseContext*
        Pointer to C++ class PythonParseContext.
    syntax_rules : dict
        Dictionary of rules for syntax expansion, which is {rule_id -> syntax_expand_func}.
    macro_rules : dict
        Dictionary of rules for macros expansion, which is {rule_id -> macro_expand_func}.
    token_rules : dict
        Dictionary of rules for token expansion, which is {rule_id -> token_expand_func}.
    exported_funcs : list
        List of names of python function exported by gimport from current module. 
        Names of these functions are used without any additional prefixes in quasiquotes.
    exported_syntax_rules : list
        List of syntax rules exported by gimport from current module.
    exported_macros_rules : list
        List of macro rules exported by gimport from current module.
    exported_tokens_rules : list
        List of token rules exported by gimport from current module.
    exported_lexer_rules : list
        List of lexer rules exported by gimport from current module. 
        Can be used for token declaration.
    exported_pyexpand_rules : list
        List of python grammar expansion rules exported by gimport from current module.
        Can be used for specific expansion of built-in python grammar rules.
    imported_modules : set
        Set of IDs of modules imported by gimport in current context.
    global_vars : dict
        Dictionary of all variables defined in current module (are from __dict__).
    parent_context : ParseContext
        Context of parent module to which current module would be imported.
    """

    cdef PythonParseContext* _ptr
    cdef dict syntax_rules
    cdef dict macro_rules
    cdef dict token_rules
    cdef list exported_funcs 
    cdef list exported_syntax_rules 
    cdef list exported_macros_rules
    cdef list exported_tokens_rules
    cdef list exported_lexer_rules
    cdef list exported_pyexpand_rules
    cdef set imported_modules
    cdef dict global_vars
    cdef object parent_context
    cdef bool allow_rule_override

    def __cinit__(self):
        """Create pointer to C++ class PythonParseContext"""
        self._ptr = create_python_context(True, python_grammar_str)

    def __dealloc__(self):
        """Delete pointer to C++ class PythonParseContext"""
        if self._ptr != NULL:
            del_python_context(self._ptr)
            self._ptr = NULL

    def __init__(self, global_vars=None):
        """
        Class constructor

        Parameters
        ----------
        global_vars : dict
            Predefined global variables.
        """
        self.allow_rule_override = False
        self.syntax_rules = {}
        self.macro_rules = {}
        self.token_rules = {}
        self.exported_syntax_rules = []
        self.exported_pyexpand_rules = []
        self.exported_macros_rules = []
        self.exported_tokens_rules = []
        self.exported_lexer_rules = []
        self.exported_funcs = []
        self.imported_modules = set()
        self.global_vars = global_vars or {}
        self.parent_context = None

    def __enter__(self):
        """Implement ParseContext object at the start of with block"""
        if self._ptr == NULL:
            raise InvalidParseContext()

        global __parse_context__
        if __parse_context__ is not None and self.parent_context is not None:
            raise Exception('Enter parse context when previous context not deleted')
        self.parent_context = __parse_context__
        __parse_context__ = self
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Implement ParseContext object at the end of with block"""
        global __parse_context__
        __parse_context__ = self.parent_context
        self.parent_context = None
        #del_python_context(self._ptr)
        #self._ptr = NULL

    cdef PythonParseContext* to_ptr(self):
        """Get PythonParseContext pointer of ParseContext object"""
        return self._ptr

    def globals(self):
        """
        Get global variables defined in current context.

        Returns
        ----------
        dict
            Dictionary of all global variables defined in current context.
        """
        return self.global_vars

    def eval(self, expr):
        """
        Evaluate expression in current context.

        Parameters
        ----------
        expr : str or ParseNode
            Expression in string or parse tree representation to be evaluated.

        Returns
        ----------
        int
            Result evaluated from the expression.
        """
        if type(expr) is ParseNode:
            expr = self.ast_to_text(expr)
        return eval(expr, self.global_vars)

    def is_valid(self):
        """ Checks whether context is valid """
        return self._ptr != NULL

    cdef object syntax_expand_func(self, int rule_id):
        """
        Get syntax expansion function of given syntax rule ID in current context.

        Parameters
        ----------
        rule_id : int
            Syntax rule ID.

        Returns
        ----------
        Callable object
            Syntax expansion function.
        """
        return self.syntax_rules.get(rule_id, None)

    cdef object macro_expand_func(self, int rule_id):
        """
        Get macro expansion function of given macro rule ID in current context.

        Parameters
        ----------
        rule_id : int
            Macro rule ID.

        Returns
        ----------
        Callable object
            Macro expansion function.
        """
        return self.macro_rules.get(rule_id, None)

    cdef object token_expand_func(self, int rule_id):
        """
        Get token expansion function of given token rule ID in current context.

        Parameters
        ----------
        rule_id : int
            Token rule ID.

        Returns
        ----------
        Callable object
            Token expansion function.
        """
        return self.token_rules.get(rule_id, None)

    def import_module(self, module):
        """
        Check is given module imported by gimport in current context, and 
        add it to set of imported modules.

        Parameters
        ----------
        module : object
            Module id which is _import_grammar function of given module.

        Returns
        ----------
        bool
            True if given module was imported by gimport in current context, 
            and False otherwise.
        """
        if module in self.imported_modules:
            return True
        self.imported_modules.add(module)
        return False

    cpdef add_token_rule(
        self, string name, string rhs, apply=None, bool for_export=False
    ):
        """
        Add token rule to current context.

        Parameters
        ----------
        name : string
            Token name.
        rhs : string
            Right hand side of token rule.
        apply : callable object
            Token expansion function whose arguments are ParseNode objects (children
            of parse node obtained by given rule).
        for_export : bool
            If true, add token rule to list of ones exported by gimport 
            from current module.
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        cdef int rule_id = add_token(self._ptr, name, rhs)
        self.token_rules[rule_id] = apply
        if for_export:
            self.exported_tokens_rules.append((name, rhs, apply))

    cpdef add_lexer_rule(self, string lhs, string rhs, bool for_export=False):
        """
        Add lexer rule to current context.

        Parameters
        ----------
        lhs : string
            Left hand side of lexer rule.
        rhs : string
            Right hand side of lexer rule.
        for_export : bool
            If true, add lexer rule to list of ones exported by gimport 
            from current module.
            
        Raises    
        ------
        InvalidParseContext
            If function called not during pyg module import
        ValueError
            If token with this name already exist
            If error in parsing expression             
        """
        cdef int rule_id
        if self._ptr == NULL:
            raise InvalidParseContext()
        try:
            rule_id = add_lexer_rule(self._ptr, lhs, rhs)
            if for_export:
                self.exported_lexer_rules.append((lhs, rhs))
        except RuntimeError as e:  # Exception thrown if token already exists of invalid PEG rule
            msg = e.args[0]
        raise ValueError(msg)

    def add_export_func(self, func):
        """
        Add python function exported by gimport from current module to current context.

        Parameters
        ----------
        func : object or str
            Python callable object or string represented name of python function.
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        self.exported_funcs.append(func.__name__ if callable(func) else func)

    def add_macro_rule(
        self, lhs: str, rhs: list, apply, for_export=False, 
        int lpriority=-1, int rpriority=-1
    ):
        """
        Add macro rule to current context.

        Parameters
        ----------
        lhs : str
            Left hand side of macro rule.
        rhs : list of str
            Right hand side of macro rule.
        apply : callable object
            Macro expansion function whose arguments are ParseNode objects (children
            of parse node obtained by given rule).
        for_export : bool
            If true, add macro rule to list of ones exported by gimport 
            from current module.
        lpriority : bool
            Left priority of binary operation or -1
        rpriority : bool
            Right priority of binary operation or -1
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        cdef int rule_id = self._add_rule(lhs, rhs, lpriority, rpriority)

        if rule_id in self.macro_rules or rule_id in self.syntax_rules:
            if not self.allow_rule_override:
                raise Exception(f'Rule already exists: {lhs} -> {" ".join(rhs)}')

        self.macro_rules[rule_id] = apply
        if for_export:
            self.exported_macros_rules.append((lhs, rhs, apply, lpriority, rpriority))

    def rule_id(self, lhs: str, rhs: list) -> int:
        """
        Get ID of macro or syntax rule in current context.

        Parameters
        ----------
        lhs : str
            Left hand side of rule.
        rhs : list of str
            Right hand side of rule.

        Returns
        ----------
        int
            Nonnegative integer represented the ID of given rule.
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        cdef int id = self._ptr.grammar().ruleId(lhs, rhs)
        if id < 0:
            raise ValueError(f'No rule: {lhs} -> {" ".join(rhs)}')
        return id

    def add_pyexpand_rule(self, lhs: str, rhs: list, apply, for_export=False):
        """
        Add python grammar expansion rule to current context.

        Parameters
        ----------
        lhs : str
            Left hand side of rule.
        rhs : list of str
            Right hand side of rule.
        apply : callable object
            Expansion function whose arguments are ParseNode objects (children of parse
            node obtained by given rule).
        for_export : bool
            If true, add rule to list of ones exported by gimport from current module.
        lpriority : bool
            Left priority of binary operation or -1
        rpriority : bool
            Right priority of binary operation or -1
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        cdef int rule_id = self.rule_id(lhs, rhs)

        if rule_id in self.macro_rules or rule_id in self.syntax_rules:
            if not self.allow_rule_override:
                raise Exception(f'Redefinition of rule expansion: {lhs} -> {" ".join(rhs)}')

        self.syntax_rules[rule_id] = apply
        if for_export:
            self.exported_pyexpand_rules.append((lhs, rhs))

    def add_syntax_rule(
        self, lhs: str, rhs : list, apply, for_export=False, lpriority=-1, rpriority=-1
    ):
        """
        Add syntax rule to current context.

        Parameters
        ----------
        lhs : str
            Left hand side of syntax rule.
        rhs : list of str
            Right hand side of syntax rule.
        apply : callable object
            Syntax expansion function whose arguments are ParseNode objects (children
            of parse node obtained by given rule).
        for_export : bool
            If true, add syntax rule to list of ones exported by gimport 
            from current module.
        lpriority : bool
            Left priority of binary operation or -1
        rpriority : bool
            Right priority of binary operation or -1
        """
        if self._ptr == NULL:
            raise InvalidParseContext()
        cdef int rule_id = self._add_rule(lhs, rhs, lpriority, rpriority)

        if rule_id in self.macro_rules or rule_id in self.syntax_rules:
            if not self.allow_rule_override:
                raise Exception(f'Rule already exists: {lhs} -> {" ".join(rhs)}')

        self.syntax_rules[rule_id] = apply
        if for_export:
            self.exported_syntax_rules.append((lhs, rhs, apply, lpriority, rpriority))

    def import_grammar_str(self):
        """
        Get string representation of import grammar function of current module
        in current context.

        Returns
        ----------
        str
            String representation of import grammar function of current module.
        """
        res = '''
def _import_grammar(module_name=None):

    # Take current context
    ctx = parse_context()

    # Check whether this function were called in current context
    if ctx.import_module(_import_grammar):
        return
    
    # Import grammars from all submodules
    if '_imported_syntax_modules' in globals():
        for submodule in _imported_syntax_modules:
            if hasattr(submodule, '_import_grammar'):
                submodule._import_grammar(ctx)
'''
        for lhs, rhs, apply, lpr, rpr in self.exported_syntax_rules:
            res += f'''
    ctx.add_syntax_rule({repr(lhs)}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})
'''
        for lhs, rhs, apply, lpr, rpr in self.exported_pyexpand_rules:
            res += f'''
    ctx.add_pyexpand_rule({repr(lhs)}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})
'''
        for lhs, rhs, apply, lpr, rpr in self.exported_macros_rules:
            res += f'''
    ctx.add_macro_rule({repr(lhs)}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})
'''
        for lhs, rhs, apply in self.exported_tokens_rules:
            res += f'''
    ctx.add_token_rule({repr(lhs)}, {repr(rhs)}, apply={apply.__name__ if apply else 'None'})
'''
        for lhs, rhs in self.exported_lexer_rules:
            res += f'''
    ctx.add_lexer_rule({repr(lhs)}, {repr(rhs)})
'''
        res += '''
    module_vars = ctx.globals()
'''
        for f in self.exported_funcs:
            res += f'''
    module_vars[{repr(f)}] = {f}
'''
        return res

    cpdef ast_to_text(self, ParseNode node):
        """
        Get string representation of node in parse tree.

        Parameters
        ----------
        node : ParseNode
            Node in parse tree

        Returns
        -------
        string
            String representation of parse node.
        """
        if self._ptr == NULL:
            raise InvalidParseContext()

        return c_ast_to_text(self._ptr, node.to_ptr())

    cdef int _add_rule(self, string lhs, vector[string] rhs, int lpr, int rpr):
        """
        Add syntax expansion rule to grammar of current context.

        Parameters
        ----------
        lhs : string
            Left hand side of rule.
        rhs : vector[string]
            Right hand side of rule.
        lpr : bool
            Left priority of binary operation or -1
        rpr : bool
            Right priority of binary operation or -1

        Returns
        -------
        int
            ID of added rule.
        """
        return self._ptr.grammar().addRule(lhs, rhs, -1, lpr, rpr)

    def rule_override(self, allow):
        self.allow_rule_override = allow

    def sprint_rules(self):
        """
        Prints grammar rules to string

        Returns
        -------
        string
            All rules printed in text format
        """
        return self._ptr.grammar().sprint_rules()


cdef class ParseIterator:
    """
    Class for iterating parser over blocks of text in given context.

    Attributes
    ----------
    state : ParserState*
        Pointer to C++ class ParserState.
    """

    cdef ParserState* state
    cdef object ctx

    def __init__(self, text : str, ctx : ParseContext):
        """
        Class constructor

        Parameters
        ----------
        text : str
            Text for parsing.
        ctx : ParseContext
            Context for parsing.
        """
        self.ctx = ctx
        self.state = new ParserState(ctx.to_ptr(), text, b"")

    def __dealloc__(self):
        """Class destructor"""
        del self.state

    def __iter__(self):
        """Iterator constructor"""
        return self

    def __next__(self):
        """Next item from the iterator container"""
        cdef CParseNode * root
        try:
            root = self.state.parse_next().root.get()
            if not root:
                raise StopIteration
            return ParseNode.from_ptr(root, self.ctx)
        except RuntimeError as e:
            msg = e.args[0]
        raise SyntaxError(msg)


def parse(text : str, ctx : ParseContext):
    """
    Generator function for parsing text by blocks in given context.

    Parameters
    ----------
    text : str
        Text for parsing.
    ctx : ParseContext
        Context for parsing.

    Returns
    -------
    ParseNode
        Node of parse tree corresponding to parsed block of text.
    """
    for node in ParseIterator(text, ctx):
        yield node


cpdef macro_expand(ParseContext ctx, ParseNode node):
    """
    Expand macro extensions for given parse node in given context.

    Parameters
    ----------
    ctx : ParseContext
        Context for parsing.
    node : ParseNode
        Node of parse tree.

    Returns
    -------
    User defined type or str
        User defined representation of expanded parse node.
    """

    # Expand macro extensions for given parse node
    while True:
        f = ctx.macro_expand_func(node.rule())
        if f is None:
            break
        node = f(*node.children())

    # Expand macro extensions for children of given parse node
    cdef int i
    for i in range(node.num_children()):
        node[i] = macro_expand(ctx, node[i])

    return node


cpdef syn_expand(node: ParseNode):
    """
    Expand syntax extensions for given parse node in default context.

    Parameters
    ----------
    node : ParseNode
        Node of parse tree.

    Returns
    -------
    User defined type or str
        User defined representation of expanded parse node.
    """
    cdef ParseContext ctx = __parse_context__
    if node.is_terminal():
        # Expand token
        apply = ctx.token_expand_func(node.ntnum())
        return node.str() if apply is None else apply(node.str())

    f = ctx.syntax_expand_func(node.rule())
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function "\
            "for rule {node.rule()}')

    return f(*node.children())


cpdef quasiquote(string name, vector[string] frags, tree_list):
    """
    Expand quasiquote in default context.

    Parameters
    ----------
    name : string
        Name of quasiquote starting nonterminal.
    frags : vector[string]
        List of quasiquote fragments.
    tree_list : list of ParseNode
        List of parse nodes corresponding to quasiquote metavariables.

    Returns
    -------
    ParseNode
        Node of parse tree obtained after quasiquote expanding.
    """
    assert frags.size() == len(tree_list) + 1
    cdef ParseContext ctx = __parse_context__
    cdef vector[CParseNode*] subtrees
    cdef size_t i, n = len(tree_list)
    cdef ParseNode pn
    if ctx is None:
        raise InvalidParseContext()
    subtrees.resize(n)
    for i in range(n):
        pn = tree_list[i]
        subtrees[i] = pn.to_ptr()
    cdef CParseNode* nn = c_quasiquote(ctx.to_ptr(), name, frags, subtrees)
    return ParseNode.from_ptr(nn, ctx)


cdef string ast_to_text(ParseNode node):
    """
    Get string representation of parse node in default context.

    Parameters
    ----------
    node : ParseNode
        Node in parse tree

    Returns
    -------
    string
        String representation of parse node.
    """
    return __parse_context__.ast_to_text(node)


def eval_in_context(expr):
    """
    Evaluate expression in default context.

    Parameters
    ----------
    expr : str or ParseNode
        Expression in string or parse tree representation to be evaluated.

    Returns
    ----------
    int
        Result evaluated from the expression.
    """
    return parse_context().eval(expr)


class CppDbgFlags:
    """Parser debug flags"""
    SHIFT = 0x1
    REDUCE = 0x2
    STATE = 0x4
    LOOKAHEAD = 0x8
    TOKEN = 0x10
    RULES = 0x20
    QQ = 0x40
    ALL = 0xFFFFFFF


def set_debug(flag : int):
    """
    Set parser debug flag

    Parameters
    ----------
    flag : int
        Parser debug flag from CppDbgFlags.
    """
    set_cpp_debug(flag)
