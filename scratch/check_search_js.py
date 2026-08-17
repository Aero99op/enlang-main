with open("scratch/compiled_youtube.js", "r", encoding="utf-8") as f:
    content = f.read()

for line in content.splitlines():
    if "searchLiveYouTube" in line or "qWords" in line or "matches" in line:
        print(line)
