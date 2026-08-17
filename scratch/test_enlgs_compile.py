import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

try:
    js = compile_enlgs_file("youtube.enlgs")
    print(f"compile_enlgs_file SUCCESS! Length: {len(js)} bytes")
    with open("scratch/compiled_youtube.js", "w", encoding="utf-8") as f:
        f.write(js)
    print("Contains hideAllViews:", "hideAllViews" in js)
    print("Contains openShortsReel:", "openShortsReel" in js)
except Exception as e:
    import traceback
    traceback.print_exc()
