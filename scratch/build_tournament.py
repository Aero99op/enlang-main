import sys
sys.path.insert(0, ".")
from enlgf.server import compile_enlgf_file

try:
    html = compile_enlgf_file("tournament_app/tournament.enlgf")
    with open("tournament_app/tournament.html", "w", encoding="utf-8") as f:
        f.write(html)
    print(f"SUCCESS! Built tournament_app/tournament.html ({len(html)} bytes)")
except Exception as e:
    import traceback
    traceback.print_exc()
