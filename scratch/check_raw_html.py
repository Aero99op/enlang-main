import sys
sys.path.insert(0, ".")
from enlgf.server import compile_enlgf_source

with open("youtube.enlgf", "r", encoding="utf-8") as f:
    source = f.read()

html = compile_enlgf_source(source)
print("Emitted HTML has script tag:")
for line in html.splitlines():
    if "script" in line:
        print(" ", line)
