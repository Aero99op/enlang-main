import os
import sys
import shutil

workspace_root = os.path.abspath(".")
if workspace_root not in sys.path:
    sys.path.insert(0, workspace_root)

from enlgd.compiler import compile_enlgd_file
from enlgs.compiler import compile_enlgs_file
from enlgf.server import compile_enlgf_source

vercel_dir = os.path.join(workspace_root, "tournament_vercel")
os.makedirs(vercel_dir, exist_ok=True)

tourney_dir = os.path.join(workspace_root, "tournament_app")
enlgd_path = os.path.join(tourney_dir, "tournament.enlgd")
enlgs_path = os.path.join(tourney_dir, "tournament.enlgs")
enlgf_path = os.path.join(tourney_dir, "tournament.enlgf")

css_dest = os.path.join(vercel_dir, "style.css")
js_dest = os.path.join(vercel_dir, "app.js")
html_dest = os.path.join(vercel_dir, "index.html")
vercel_json_dest = os.path.join(vercel_dir, "vercel.json")

print("1. Compiling stylesheet -> tournament_vercel/style.css ...")
css_code = compile_enlgd_file(enlgd_path)
with open(css_dest, "w", encoding="utf-8") as f:
    f.write(css_code)
print(f"   [OK] Written {css_dest} ({len(css_code)} bytes)")

print("2. Compiling script -> tournament_vercel/app.js ...")
js_code = compile_enlgs_file(enlgs_path)
with open(js_dest, "w", encoding="utf-8") as f:
    f.write(js_code)
print(f"   [OK] Written {js_dest} ({len(js_code)} bytes)")

print("3. Compiling markup -> tournament_vercel/index.html ...")
with open(enlgf_path, "r", encoding="utf-8") as f:
    enlgf_source = f.read()

raw_html = compile_enlgf_source(enlgf_source)
if "</head>" in raw_html:
    raw_html = raw_html.replace("</head>", "    <link rel=\"stylesheet\" href=\"style.css\">\n  </head>")
if "</body>" in raw_html:
    raw_html = raw_html.replace("</body>", "    <script src=\"app.js\"></script>\n  </body>")

with open(html_dest, "w", encoding="utf-8") as f:
    f.write(raw_html)
print(f"   [OK] Written {html_dest} ({len(raw_html)} bytes)")

print("4. Creating tournament_vercel/vercel.json ...")
vercel_config = """{
  "version": 2,
  "cleanUrls": true,
  "headers": [
    {
      "source": "/(.*)",
      "headers": [
        {
          "key": "Cache-Control",
          "value": "public, max-age=0, must-revalidate"
        }
      ]
    }
  ]
}
"""
with open(vercel_json_dest, "w", encoding="utf-8") as f:
    f.write(vercel_config)
print(f"   [OK] Written {vercel_json_dest}")

print("\n=== TOURNAMENT VERCEL DIRECTORY SETUP COMPLETE! ===")
