import urllib.request
import json

try:
    req = urllib.request.Request("https://api.invidious.io/instances.json?sort_by=health", headers={'User-Agent': 'Mozilla/5.0'})
    res = urllib.request.urlopen(req, timeout=5)
    instances = json.loads(res.read().decode('utf-8'))
    
    working = []
    for item in instances:
        domain = item[0]
        meta = item[1]
        uri = meta.get('uri')
        if uri:
            test_url = f"{uri}/api/v1/trending?region=IN"
            try:
                t_req = urllib.request.Request(test_url, headers={'User-Agent': 'Mozilla/5.0'})
                t_res = urllib.request.urlopen(t_req, timeout=3)
                data = json.loads(t_res.read().decode('utf-8'))
                if isinstance(data, list) and len(data) > 0:
                    print(f"SUCCESS: {uri} (returned {len(data)} videos)")
                    working.append(uri)
            except Exception as e:
                pass
    print("Working endpoints:", working)
except Exception as e:
    print("Error:", e)
