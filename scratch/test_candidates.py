import urllib.request
import json
import ssl
import sys

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

candidates = [
    "https://invidious.flokinet.to/api/v1/search?q={q}&type=video",
    "https://inv.in.projectsegfau.lt/api/v1/search?q={q}&type=video",
    "https://iv.melmac.space/api/v1/search?q={q}&type=video",
    "https://invidious.private.coffee/api/v1/search?q={q}&type=video",
    "https://yewtu.be/api/v1/search?q={q}&type=video",
    "https://invidious.einfachzocken.eu/api/v1/search?q={q}&type=video",
    "https://invidious.esmailelbob.xyz/api/v1/search?q={q}&type=video",
    "https://pipedapi.ducks.party/search?q={q}&filter=videos",
    "https://pipedapi.leptons.xyz/search?q={q}&filter=videos",
    "https://pipedapi.r4fo.com/search?q={q}&filter=videos",
    "https://pipedapi.nosebs.ru/search?q={q}&filter=videos",
    "https://api.piped.privacydev.net/search?q={q}&filter=videos"
]

for url_tpl in candidates:
    url = url_tpl.format(q=urllib.parse.quote("snax gaming"))
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, context=ctx, timeout=3) as res:
            raw = res.read().decode('utf-8', errors='ignore')
            data = json.loads(raw)
            items = data if isinstance(data, list) else data.get('items', [])
            print(f"[SUCCESS] {url_tpl.split('/')[2]}: {len(items)} items")
            if items:
                print(f"   -> {items[0].get('title')}")
    except Exception as e:
        print(f"[FAIL] {url_tpl.split('/')[2]}: {e}")
