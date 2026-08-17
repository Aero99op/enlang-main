import urllib.request
import json

payload = {
    "context": {
        "client": {
            "clientName": "WEB",
            "clientVersion": "2.20240101.01.00",
            "gl": "IN",
            "hl": "en"
        }
    },
    "browseId": "FEwhat_to_watch" # YouTube Home / Trending Browse ID!
}

req = urllib.request.Request(
    "https://www.youtube.com/youtubei/v1/browse",
    data=json.dumps(payload).encode('utf-8'),
    headers={
        'Content-Type': 'application/json',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
    }
)

try:
    res = urllib.request.urlopen(req, timeout=6)
    data = json.loads(res.read().decode('utf-8'))
    print("[INNERTUBE SUCCESS!] Response keys:", list(data.keys()))
    # Let's check contents
    contents = data.get('contents', {})
    print("Found live YouTube Home/Trending Feed without API key!")
except Exception as e:
    print("[INNERTUBE ERROR]:", e)
