import sys
import os
import subprocess

# Set stdout/stderr to UTF-8
if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

try:
    js_code = compile_enlgs_file("youtube.enlgs")
    with open("scratch/temp_check.js", "w", encoding="utf-8") as f:
        f.write(js_code)
    
    res = subprocess.run(["node", "--check", "scratch/temp_check.js"], capture_output=True, text=True, encoding="utf-8")
    if res.returncode == 0:
        print("PERFECT! 0 SYNTAX ERRORS! Node check passed cleanly!")
    else:
        print("NODE ERROR:\n", res.stderr)
except Exception as e:
    import traceback
    traceback.print_exc()
