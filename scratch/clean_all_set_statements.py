import re

with open("tournament_app/tournament.enlgs", "r", encoding="utf-8") as f:
    content = f.read()

lines = content.splitlines()
new_lines = []

for line in lines:
    stripped = line.strip()
    l_indent = line[:len(line) - len(line.lstrip())]
    
    # Check if this line is an assignment without 'set' or 'create' or 'define'
    # Examples: htmlBuffer = htmlBuffer + ..., record.kills = ..., teamMap[...] = ..., targetScore[field] = ...
    if "=" in stripped and not stripped.startswith(("#", "create ", "define ", "set ", "if ", "when ", "for each ", "window.")):
        parts = stripped.split("=", 1)
        lhs = parts[0].strip()
        # Make sure lhs looks like an identifier or property path (e.g. htmlBuffer, record.kills, teamMap[k], optBuffer)
        if re.match(r'^[a-zA-Z_][a-zA-Z0-9_\.\[\]\"\']*\s*$', lhs):
            new_lines.append(f"{l_indent}set {stripped}")
            continue
            
    new_lines.append(line)

new_content = "\n".join(new_lines) + "\n"

with open("tournament_app/tournament.enlgs", "w", encoding="utf-8") as f:
    f.write(new_content)

print("Updated tournament.enlgs with pure 'set' statements everywhere!")
