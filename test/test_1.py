import sys
import pymacros.core.wrap as parser

if __name__ == "__main__":

    print("new parser context ... ", end="", flush=True)
    context = parser.new_python_context(0, sys.argv[1])
    print("ok")

    print("new parser state ... ", end="", flush=True)
    state = parser.new_parser_state(context, "", "")
    print("ok")

    print("delete parser context ... ", end="", flush=True)
    parser.del_python_context(context)
    print("ok")

    print("delete parser state ... ", end="", flush=True)
    parser.del_parser_state(state)
    print("ok")