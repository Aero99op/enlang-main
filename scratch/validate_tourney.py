import re
import subprocess

with open("tournament_app/tournament.html", "r", encoding="utf-8") as f:
    html = f.read()

scripts = re.findall(r'<script>(.*?)</script>', html, re.DOTALL)
print(f"Found {len(scripts)} scripts in tournament.html")

for idx, js in enumerate(scripts):
    with open("scratch/temp_tourney_check.js", "w", encoding="utf-8") as f:
        f.write(js)
    res = subprocess.run(["node", "--check", "scratch/temp_tourney_check.js"], capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error in script {idx}:\n{res.stderr}")
    else:
        print(f"Script {idx}: SYNTAX OK!")
