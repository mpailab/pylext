from random import choice, randint, sample
from grammar import CFG
from tree import Tree

def run (grammar, depth, size = 1000000):
    assert ( isinstance(grammar, CFG) and isinstance(depth, int) and isinstance(size, int) )
    skip_prods = grammar.skip_prods()
    tree = Tree(grammar.start, grammar.indent, grammar. dedent)
    while tree.size < size:
        leaves = tree.leaves(depth)
        if leaves:
            k = randint(1, int(len(leaves) / 2)) if len(leaves) > 5 else 1
            indexes = sample(range(len(leaves)), k)
            for i in indexes:
                node = leaves[i]
                prod = choice(grammar.symbol_prods(node.symbol))
                tree.insert(node, prod)
        else:
            skip_nodes = tree.skips
            if skip_prods and skip_nodes:
                k = randint(1, int(len(skip_nodes) / 2)) if len(skip_nodes) > 5 else 1
                indexes = sample(range(len(skip_nodes)), k)
                for i in indexes:
                    node = skip_nodes[i]
                    prod = choice([ x for x in skip_prods[node.symbol] if node.symbol in x.rhs ])
                    tree.insert(node, prod)
            else:
                break
    leaves = tree.leaves()
    for node in leaves[:]:
        prod = choice(grammar.symbol_ground(node.symbol))
        tree.insert(node, prod)
    return tree
    