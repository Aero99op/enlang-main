import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

try:
    js = compile_enlgs_file("youtube.enlgs")
    print("SUCCESS! Compiled JS length:", len(js))
    print("JS Preview:\n", js[:500])
except Exception as e:
    import traceback
    traceback.print_exc()
