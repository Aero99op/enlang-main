with open("scratch/compiled_youtube.js", "r", encoding="utf-8") as f:
    lines = f.readlines()

for i in range(max(0, 390), min(len(lines), 410)):
    print(f"{i+1}: {lines[i].rstrip()}")
