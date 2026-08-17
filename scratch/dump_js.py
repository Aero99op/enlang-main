import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

js = compile_enlgs_file("youtube.enlgs")
with open("scratch/compiled_yt.js", "w", encoding="utf-8") as f:
    f.write(js)
print("Wrote scratch/compiled_yt.js, length:", len(js))
