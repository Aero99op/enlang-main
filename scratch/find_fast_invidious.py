import urllib.request
import json
import ssl

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

instances = [
    "https://invidious.flokinet.to",
    "https://inv.tux.pizza",
    "https://invidious.projectsegfau.lt",
    "https://invidious.private.coffee",
    "https://invidious.asir.dev",
    "https://iv.ggtyler.dev",
    "https://invidious.perennialte.ch",
    "https://yt.artemislena.eu",
    "https://invidious.drgns.space",
    "https://invidious.vern.cc"
]

for inst in instances:
    url = f"{inst}/api/v1/search?q=snax%20gaming&type=video"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, context=ctx, timeout=4) as res:
            data = json.loads(res.read().decode('utf-8', errors='ignore'))
            print(f"[FAST WORKING] {inst}: {len(data)} items returned!")
            for item in data[:2]:
                print(f"   -> {item.get('title')} ({item.get('author')}) - {item.get('videoId')}")
    except Exception as e:
        print(f"[FAILED] {inst}: {e}")
