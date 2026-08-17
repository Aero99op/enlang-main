import urllib.request
import json
import ssl

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

url = "https://api.invidious.io/instances.json?sort_by=type,health"
req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
with urllib.request.urlopen(req, context=ctx, timeout=8) as res:
    data = json.loads(res.read().decode('utf-8'))
    for item in data[:8]:
        print(item[0], item[1].get('uri'), item[1].get('api'))
