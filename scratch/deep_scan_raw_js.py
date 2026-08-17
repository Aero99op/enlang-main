with open("youtube.enlgs", "r", encoding="utf-8") as f:
    lines = f.readlines()

print(f"Total lines in youtube.enlgs: {len(lines)}")
raw_js_patterns = [
    "document.",
    "function(",
    ".innerHTML",
    ".innerText",
    ".textContent",
    ".style.",
    ".addEventListener",
    ".onclick",
    ".onkeyup",
    ".oninput",
    ".filter(",
    ".map(",
    ".forEach(",
    "fetch(",
    "JSON.stringify",
    "Math.",
    ".appendChild",
    ".prepend"
]

found = []
for i, line in enumerate(lines, 1):
    for pat in raw_js_patterns:
        if pat in line:
            found.append((i, pat, line.strip()))
            break

print(f"Found {len(found)} lines with raw JS patterns:")
for line_num, pat, content in found:
    print(f"Line {line_num:3d} [{pat}]: {content}")
