import urllib.request
import json

endpoints = [
    "https://pipedapi.kavin.rocks/trending?region=IN",
    "https://api.piped.video/trending?region=IN",
    "https://inv.tux.pizza/api/v1/trending?region=IN",
    "https://yewtu.be/api/v1/trending?region=IN"
]

for ep in endpoints:
    try:
        req = urllib.request.Request(ep, headers={'User-Agent': 'Mozilla/5.0'})
        res = urllib.request.urlopen(req, timeout=5)
        data = json.loads(res.read().decode('utf-8'))
        print(f"[SUCCESS] {ep} returned {len(data)} items!")
        if len(data) > 0:
            item = data[0]
            print(" Sample:", item.get('title') or item.get('url'))
    except Exception as e:
        print(f"[FAILED] {ep}: {e}")
