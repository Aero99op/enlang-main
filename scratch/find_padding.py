import re
with open("portfolio.html", "r", encoding="utf-8") as f:
    for i, line in enumerate(f):
        if "padding: 80px 0;" in line and "<" not in line and ">" not in line:
            print(f"Line {i+1}: {line.strip()}")
