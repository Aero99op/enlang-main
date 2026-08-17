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

q = "snax gaming"
for page in [1, 2]:
    url = f"https://invidious.flokinet.to/api/v1/search?q={urllib.parse.quote(q)}&page={page}&type=video"
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req, context=ctx, timeout=6) as res:
        data = json.loads(res.read().decode('utf-8'))
        print(f"Page {page} for query '{q}' returned {len(data)} items:")
        for v in data[:2]:
            print(f"  - [{v.get('videoId')}] {v.get('title')[:50]}")
