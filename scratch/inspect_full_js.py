import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

js = compile_enlgs_file("youtube.enlgs")
with open("scratch/dump_full.js", "w", encoding="utf-8") as f:
    f.write(js)

lines = js.splitlines()
print("Total JS lines:", len(lines))
for i in range(max(0, len(lines)-40), len(lines)):
    print(f"{i+1}: {repr(lines[i])}")
