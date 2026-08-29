with open("tournament_app/tournament.enlgs", "r", encoding="utf-8") as f:
    lines = f.readlines()

valid_starters = (
    "in script:", "create ", "define ", "set ", "get ", "when ", "to do ",
    "call ", "if ", "else", "for each ", "show element ", "hide element ",
    "add class ", "remove class ", "scroll to ", "copy ", "after ", "return ",
    "#", "window.", "tempTeamScores."
)

for i, line in enumerate(lines):
    s = line.strip()
    if not s:
        continue
    is_valid = any(s.startswith(v) for v in valid_starters) or s in ("else:", "in script:")
    if not is_valid:
        print(f"Line {i+1}: {s.encode('ascii', errors='replace').decode('ascii')}")
