import sys
import os

# Add workspace root to PYTHONPATH
workspace_root = os.path.abspath(".")
if workspace_root not in sys.path:
    sys.path.insert(0, workspace_root)

from enlgd.compiler import compile_enlgd_file
from enlgs.compiler import compile_enlgs_file
from enlgf.server import compile_enlgf_file, compile_enlgf_source

tourney_dir = os.path.join(workspace_root, "tournament_app")
enlgd_path = os.path.join(tourney_dir, "tournament.enlgd")
enlgs_path = os.path.join(tourney_dir, "tournament.enlgs")
enlgf_path = os.path.join(tourney_dir, "tournament.enlgf")

css_out_path = os.path.join(tourney_dir, "tournament.css")
js_out_path = os.path.join(tourney_dir, "tournament.js")
html_bundled_path = os.path.join(tourney_dir, "tournament.html")
html_modular_path = os.path.join(tourney_dir, "index.html")

print("1. Compiling tournament.enlgd -> tournament.css ...")
css_code = compile_enlgd_file(enlgd_path)
with open(css_out_path, "w", encoding="utf-8") as f:
    f.write(css_code)
print(f"   [OK] Written {css_out_path} ({len(css_code)} bytes)")

print("2. Compiling tournament.enlgs -> tournament.js ...")
js_code = compile_enlgs_file(enlgs_path)
with open(js_out_path, "w", encoding="utf-8") as f:
    f.write(js_code)
print(f"   [OK] Written {js_out_path} ({len(js_code)} bytes)")

print("3. Compiling tournament.enlgf -> tournament.html (bundled) ...")
bundled_html = compile_enlgf_file(enlgf_path, style_path=enlgd_path, script_path=enlgs_path)
with open(html_bundled_path, "w", encoding="utf-8") as f:
    f.write(bundled_html)
print(f"   [OK] Written {html_bundled_path} ({len(bundled_html)} bytes)")

print("4. Creating modular index.html linking tournament.css & tournament.js ...")
with open(enlgf_path, "r", encoding="utf-8") as f:
    enlgf_source = f.read()

# Replace connect design & connect script with external link and script tags
raw_html = compile_enlgf_source(enlgf_source)
# In raw_html, ensure we have <link rel="stylesheet" href="tournament.css"> in <head> and <script src="tournament.js"></script> before </body>
if "<head>" in raw_html:
    raw_html = raw_html.replace("</head>", "    <link rel=\"stylesheet\" href=\"tournament.css\">\n  </head>")
if "</body>" in raw_html:
    raw_html = raw_html.replace("</body>", "    <script src=\"tournament.js\"></script>\n  </body>")

with open(html_modular_path, "w", encoding="utf-8") as f:
    f.write(raw_html)
print(f"   [OK] Written {html_modular_path} ({len(raw_html)} bytes)")

print("\n--- ALL HTML, CSS, AND JS VERSIONS GENERATED SUCCESSFULLY! ---")
