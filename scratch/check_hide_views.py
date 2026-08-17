with open("scratch/compiled_youtube.js", "r", encoding="utf-8") as f:
    js = f.read()

lines = js.splitlines()
for i, line in enumerate(lines):
    if "function hideAllViews" in line:
        for j in range(i, min(len(lines), i + 15)):
            print(lines[j])
        break
