from . import _wrap as wrap # type: ignore

def apply (text):
    return wrap.c_apply(text.encode('utf-8')).decode('utf-8')