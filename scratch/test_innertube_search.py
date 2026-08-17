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
    "query": "Java 21 Spring Boot"
}

req = urllib.request.Request(
    "https://www.youtube.com/youtubei/v1/search",
    data=json.dumps(payload).encode('utf-8'),
    headers={
        'Content-Type': 'application/json',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
    }
)

try:
    res = urllib.request.urlopen(req, timeout=6)
    data = json.loads(res.read().decode('utf-8'))
    print("[INNERTUBE SEARCH SUCCESS!]")
    
    # Extract video titles and videoIds
    primary = data.get('contents', {}).get('twoColumnSearchResultsRenderer', {}).get('primaryContents', {}).get('sectionListRenderer', {}).get('contents', [])
    items = []
    if primary:
        contents = primary[0].get('itemSectionRenderer', {}).get('contents', [])
        for c in contents:
            v = c.get('videoRenderer')
            if v:
                vid = v.get('videoId')
                title = v.get('title', {}).get('runs', [{}])[0].get('text')
                channel = v.get('ownerText', {}).get('runs', [{}])[0].get('text')
                views = v.get('viewCountText', {}).get('simpleText')
                duration = v.get('lengthText', {}).get('simpleText')
                items.append({
                    "id": vid,
                    "title": title,
                    "channel": channel,
                    "views": views,
                    "duration": duration,
                    "thumb": f"https://i.ytimg.com/vi/{vid}/hqdefault.jpg"
                })
    print(f"Parsed {len(items)} REAL YouTube videos!")
    for it in items[:4]:
        print(f" - [{it['id']}] {it['title']} by {it['channel']} ({it['views']}, {it['duration']})")
except Exception as e:
    print("[INNERTUBE SEARCH ERROR]:", e)
