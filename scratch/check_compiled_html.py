import sys
sys.path.insert(0, ".")
from enlgf.server import compile_enlgf_file

html = compile_enlgf_file("youtube.enlgf")
print("Has <script>: ", "<script" in html)
print("Has <style>: ", "<style" in html)
if "<script" in html:
    idx = html.find("<script")
    print("Script snippet:\n", html[idx:idx+300])
else:
    print("HTML Tail:\n", html[-500:])
