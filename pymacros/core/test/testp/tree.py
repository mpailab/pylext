import re
from functools import reduce
from grammar import Symbol, Production

class Node (object):

    def __init__ (self, symbol, parent = None, depth = 0):

        assert ( isinstance(symbol, Symbol) and 
                 ( parent is None or isinstance(parent, Node) ) and
                 isinstance(depth, int) )

        self._symbol = symbol
        self._parent = parent
        self._childs = []
        self._depth = depth
        self._production = None

    def is_root (self):
        return self._parent is None

    @property
    def symbol (self):
        return self._symbol

    @property
    def parent (self):
        return self._parent

    @property
    def childs (self):
        return self._childs

    @childs.setter
    def childs (self, childs):
        assert ( isinstance(childs, list) and all ( isinstance(x, Node) for x in childs ) )
        self._childs = childs

    @property
    def depth (self):
        return self._depth

    @property
    def production (self):
        return self._production

    @production.setter
    def production (self, prod):
        assert ( isinstance(prod, Production) )
        self._production = prod

    def __str__ (self, pref='', next_pref=''):
        res = pref + ('' if next_pref == '' else '+-') + str(self._symbol) + '\n'
        for i, child in enumerate(self._childs):
            res += child.__str__(pref + next_pref, '| ' if i+1 < len(self._childs) else '  ')
        return res

    def terminals (self):
        nodes = [self]
        while any (x.childs for x in nodes):
            nodes = [ y for x in nodes for y in (x.childs if x.childs else [x]) ]
        return nodes
    
    def _to_str (self):
        prod = self._production
        if prod is None:
            return (str(self._symbol), {})

        else:
            assert (self._childs)
            childs_res = list(zip(*map(lambda x: x._to_str(), self._childs)))
            string = ' '.join(childs_res[0])
            prods = { k : v for d in childs_res[1] for k, v in d.items()}
            if prod.add:
                prods[prod] = 1

        if (self._symbol.syntax or self._parent is None) and prods:
            string = ''.join(map(lambda x: '%syntax: ' + str(x) + '\n', prods.keys())) + string
            prods = {}

        return (string, prods)
    
    def to_str (self, simple = False):
        if simple:
            string = ' '.join(list(map(lambda x: str(x.symbol), self.terminals())))
        else:
            string, prods = self._to_str()
            assert (not prods)
        string = re.sub(r'\s*\\n *', '\n', string)
        string = re.sub(r' *\\t *', '\t', string)
        return string

class Tree (object):

    def __init__ (self, symbol, indent, dedent):

        assert ( isinstance(symbol, Symbol) and 
                 isinstance(indent, Symbol) and
                 isinstance(dedent, Symbol) )

        self._root = Node(symbol)
        self._indent = indent
        self._dedent = dedent
        self._skips = [self._root] if symbol.skip else []
        self._leaves = [] if symbol.terminal else [self._root]
        self._size = 1

    @property
    def root (self):
        return self._root

    @property
    def indent (self):
        return self._indent

    @property
    def dedent (self):
        return self._dedent

    @property
    def skips (self):
        return self._skips

    def leaves (self, depth = None):
        if depth is None:
            return self._leaves
        assert ( isinstance(depth, int) )
        return [ x for x in self._leaves if x.depth < depth ]

    @property
    def size (self):
        return self._size

    def insert (self, node, prod):

        assert ( isinstance(node, Node) and not node.symbol.terminal and
                 isinstance(prod, Production) and node.symbol == prod.lhs )
        
        depth = node.depth if prod.skip else node.depth + 1
        childs = [ Node(x, node, depth) for x in prod.rhs ]

        if node.symbol in prod.rhs and node.production is not None:
            i = prod.rhs.index(node.symbol)
            child = childs[i]
            child.childs = node.childs
            child.production = node.production
            self._leaves += [ x for x in childs if not x.symbol.terminal and x.symbol not in [self.indent, self.dedent] and x != child ]
        else:
            assert (node in self._leaves)
            self._leaves.remove(node)
            self._leaves += [ x for x in childs if not x.symbol.terminal and x.symbol not in [self.indent, self.dedent] ]
            
        node.childs = childs
        node.production = prod
        self._skips += [ x for x in childs if x.symbol.skip ]
        self._size += len(childs)

    def __str__ (self):
        return str(self._root)
    
    def to_str (self, simple = False):
        indent = str(self.indent)
        dedent = str(self.dedent)
        string = self._root.to_str(simple)
        string = re.sub(' *%s *' % indent, indent, string)
        string = re.sub(' *%s *' % dedent, dedent, string)

        def add_indent (string):
            return '\n'.join(map(lambda x: '\t' + x, string.splitlines()))+'\n'

        while True:
            m = re.compile('(.*)%s(.*?)%s(.*)' % (indent, dedent), re.S).match(string)
            if m:
                string = m.group(1) + add_indent(m.group(2)) + m.group(3)
            else:
                if re.compile('.*\b(%s|%s)\b.*' % (indent, dedent), re.S).match(string):
                    raise ValueError('Expected that %s precedes %s, found: %s' % (indent, dedent, string))
                break
        return string
