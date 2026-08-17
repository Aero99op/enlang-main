import urllib.request
import json
import ssl

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

def test_endpoint(name, url_template, q):
    url = url_template.format(q=urllib.parse.quote(q))
    try:
        req = urllib.request.Request(url, headers={
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
        })
        with urllib.request.urlopen(req, context=ctx, timeout=6) as res:
            raw = res.read().decode('utf-8', errors='ignore')
            data = json.loads(raw)
            print(f"[SUCCESS] {name} returned response!")
            if isinstance(data, list):
                print(f"   Returned list with {len(data)} items:")
                for item in data[:3]:
                    if isinstance(item, dict):
                        print(f"   - Title: {item.get('title')}, Author: {item.get('author')}, ID: {item.get('videoId')}")
            elif isinstance(data, dict):
                items = data.get('items', []) or data.get('results', []) or data.get('videos', [])
                print(f"   Returned dict with {len(items)} items:")
                for item in items[:3]:
                    print(f"   - {item}")
            return True
    except Exception as e:
        print(f"[FAIL] {name}: {e}")
        return False

endpoints = [
    ("Invidious ludo", "https://invidious.lunar.icu/api/v1/search?q={q}&type=video"),
    ("Invidious drg", "https://invidious.drgns.space/api/v1/search?q={q}&type=video"),
    ("Invidious flokinet", "https://invidious.flokinet.to/api/v1/search?q={q}&type=video"),
    ("Invidious privacydev", "https://invidious.privacydev.net/api/v1/search?q={q}&type=video"),
    ("Invidious no-google", "https://invidious.no-google.pt/api/v1/search?q={q}&type=video"),
    ("Invidious privacyredirect", "https://inv.nadeko.net/api/v1/search?q={q}&type=video"),
    ("Invidious snopyta", "https://invidious.snopyta.org/api/v1/search?q={q}&type=video"),
    ("Piped kavin", "https://pipedapi.kavin.rocks/search?q={q}&filter=videos"),
    ("Piped sync", "https://pipedapi.syncpundit.io/search?q={q}&filter=videos"),
    ("Piped adminforge", "https://pipedapi.adminforge.de/search?q={q}&filter=videos"),
    ("Piped toxic", "https://pipedapi.tokhmi.xyz/search?q={q}&filter=videos"),
    ("Piped astral", "https://pipedapi.leptons.xyz/search?q={q}&filter=videos")
]

for name, template in endpoints:
    test_endpoint(name, template, "snax gaming")
