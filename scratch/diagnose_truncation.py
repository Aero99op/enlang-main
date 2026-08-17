import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file
from enlgf.server import compile_enlgf_file

js = compile_enlgs_file("youtube.enlgs")
print("Full compiled JS length:", len(js))
print("Full compiled JS line count:", len(js.splitlines()))

html = compile_enlgf_file("youtube.enlgf")
print("Full HTML length:", len(html))
print("Has when clicked in HTML:", "btn-player-play" in html)
print("HTML last 500 chars:\n", html[-500:])
