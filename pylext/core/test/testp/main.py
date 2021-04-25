import argparse, sys
from os import devnull
from random import choice, randint, shuffle
from generator import run
from grammar import CFG
from tree import Tree

DEPTH = 150
SIZE = 10000

sys.setrecursionlimit(1000000)

parser = argparse.ArgumentParser( description='generate an arbitrary string in context-free grammar',
                                  formatter_class=argparse.RawDescriptionHelpFormatter, epilog='''
Context-free grammar format \n

1. Terminal symbols have the form:
   <term> = \'[^\']+\' or \"[^\"]+\"

2. Nonterminal symbols have the form:
   <nonterm> = [\w/][\w/^<>-]*

3. Production have the form:
   <production> = <nonterm> <sep> <altsyms>, where <altsyms> = <syms> or <syms> <alt> <altsyms>
                                                   <syms>    = <sym> or <sym> <syms>
                                                   <sym>     = <term> or <nonterm>
                                                   <alt>     = |
                                                   <sep> is defined by --sep

4. Multiline productions separeted by \ are allowed.

5. Lines started with # are connents, with % are directives.

6. There are four directives

- set the start symbol of grammar:
% start <nonterm>

- set the indent token of grammar:
% indent <nonterm>

- set the dedent token of grammar:
% dedent <nonterm>

- set symbols such that additional productions can be inserted in derivation tree before subtrees corresponding these symbols:
% syntax <nonterm> ... <nonterm>

- set a production that are not taken into account when calculating the depth of derivation tree:
% skip
<production>

- set an additional production that can be inserted in derivation tree:
% add
<production>

Directives 'syntax' and 'add' are considered only for dynamic grammars mode. This mode can be turned on by --dynamic.
''')
parser.add_argument( 'file', metavar='file', type=str, help='file with grammar')
parser.add_argument( '--output', metavar='path', type=str, default='stdout', help='output file (default: %(default)s)')
parser.add_argument( '--depth', metavar='int', type=int, default=DEPTH, help='depth of derivation tree (default: %(default)s)')
parser.add_argument( '--size', metavar='int', type=int, default=SIZE, help='size of derivation tree (default: %(default)s)')
parser.add_argument( '--dynamic', action='store_true', help='save output string in dynamic grammar')
parser.add_argument( '--sep', metavar='str', type=str, default='=', help='derivation symbol in productions (default: \'%(default)s\')')
parser.add_argument( '--preamble', metavar='str', type=str, default='', help='put this string in the preamble of output file (default: \"%(default)s\")')
parser.add_argument( '--verbose', action='store_true', help='displays details about the process')
args = parser.parse_args()

with open(args.file, 'r') as file:
    stdout = sys.stdout if args.verbose else open(devnull, 'w')
    print('Read grammar ... ', file = stdout, end = '', flush = True)
    cfg = CFG.fromsource(file, args.sep)
    print('ok', file = stdout)
    print('Generate derivation tree ... ', file = stdout, end = '', flush = True)
    tree = run(cfg, args.depth, args.size)
    print('ok', file = stdout)
    print('Convert to string ... ', file = stdout, end = '', flush = True)
    if args.preamble != '':
        preamble = open(args.preamble,'r').read()+'\n'
    else: preamble = ''
    if args.output == 'stdout':
        print(preamble + tree.to_str(not args.dynamic))
    else:
        with open(args.output, 'w') as output:
            output.write(preamble + tree.to_str())
    print('ok', file = stdout)
