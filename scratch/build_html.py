import sys
sys.path.insert(0, ".")
from enlgf.server import compile_enlgf_file

try:
    html = compile_enlgf_file("youtube.enlgf")
    with open("youtube.html", "w", encoding="utf-8") as f:
        f.write(html)
    print(f"SUCCESS! Built youtube.html ({len(html)} bytes)")
except Exception as e:
    import traceback
    traceback.print_exc()
