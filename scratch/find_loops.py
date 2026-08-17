with open("scratch/compiled_youtube.js", "r", encoding="utf-8") as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "for (const item of" in line or "for (const" in line:
        print(f"{i+1}: {line.strip()}")
