import re

with open("tournament_app/tournament.enlgs", "r", encoding="utf-8") as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if re.search(r'^\s*set\s+\w+\[', line):
        print(f"Line {i+1}: {line.strip()}")
