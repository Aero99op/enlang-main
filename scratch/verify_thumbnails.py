import re
import urllib.request

with open("youtube.enlgs", "r", encoding="utf-8") as f:
    content = f.read()

ids = list(set(re.findall(r'id:\s*"([a-zA-Z0-9_-]{11})"', content)))
print(f"Found {len(ids)} unique video IDs in youtube.enlgs")

broken = []
working = []

for vid in ids:
    url = f"https://i.ytimg.com/vi/{vid}/hqdefault.jpg"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=3) as resp:
            if resp.status == 200:
                working.append(vid)
            else:
                broken.append((vid, resp.status))
    except Exception as e:
        broken.append((vid, str(e)))

print(f"Working thumbnails: {len(working)}")
print(f"Broken thumbnails: {len(broken)}")
for b in broken:
    print("  BROKEN:", b)
