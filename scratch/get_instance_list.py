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
    print(f"Total instances found: {len(data)}")
    healthy = []
    for item in data:
        domain = item[0]
        meta = item[1]
        if meta.get('type') == 'https' and meta.get('api') == True:
            uri = meta.get('uri')
            if uri:
                healthy.append(uri)
    print(f"Found {len(healthy)} API instances. Testing top 10...")
    
    for u in healthy[:10]:
        try:
            test_url = f"{u}/api/v1/search?q=snax%20gaming&type=video"
            r = urllib.request.Request(test_url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(r, context=ctx, timeout=4) as test_res:
                d = json.loads(test_res.read().decode('utf-8'))
                if isinstance(d, list) and len(d) > 0:
                    print(f"  ✓ {u} : {len(d)} real YouTube results! (e.g. {d[0].get('title')})")
        except Exception as e:
            print(f"  ✗ {u} : {e}")
