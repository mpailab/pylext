from pymacros import exec_macros

if __name__ == "__main__":
    try:
        exec_macros("""
        def f(x)):
            return
        """, {})
    except RuntimeError as err:
        print(f"RuntimeError: {err}")
