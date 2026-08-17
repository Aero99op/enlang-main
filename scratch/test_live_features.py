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

# 1. Test live comments for a real video
vid = "0e3GPea1Tyg" # MrBeast Squid Game
url = f"https://invidious.flokinet.to/api/v1/comments/{vid}"
print(f"--- 1. Testing Live YouTube Comments for {vid} ---")
try:
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req, context=ctx, timeout=6) as res:
        data = json.loads(res.read().decode('utf-8'))
        comments = data.get('comments', [])
        print(f"SUCCESS: Fetched {len(comments)} real YouTube comments!")
        for c in comments[:3]:
            print(f"  - [{c.get('author')}] ({c.get('authorThumbnails', [{}])[0].get('url')}): {c.get('content')[:80]}... (👍 {c.get('likeCount')})")
except Exception as e:
    print(f"Error fetching comments: {e}")

# 2. Test live Shorts
print(f"\n--- 2. Testing Live YouTube Shorts ---")
url_shorts = "https://invidious.flokinet.to/api/v1/search?q=%23shorts%20viral&type=video"
try:
    req = urllib.request.Request(url_shorts, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req, context=ctx, timeout=6) as res:
        data = json.loads(res.read().decode('utf-8'))
        print(f"SUCCESS: Fetched {len(data)} real YouTube shorts!")
        for s in data[:3]:
            print(f"  - [{s.get('videoId')}] {s.get('title')} by {s.get('author')} ({s.get('viewCount')} views)")
except Exception as e:
    print(f"Error fetching shorts: {e}")

# 3. Test video search with authorThumbnails (channel avatar)
print(f"\n--- 3. Testing Real Channel Avatars in Search ---")
url_search = "https://invidious.flokinet.to/api/v1/search?q=snax%20gaming&type=video"
try:
    req = urllib.request.Request(url_search, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req, context=ctx, timeout=6) as res:
        data = json.loads(res.read().decode('utf-8'))
        print(f"SUCCESS: Fetched {len(data)} videos with avatars:")
        for v in data[:3]:
            avatar = v.get('authorThumbnails', [{}])[-1].get('url') if v.get('authorThumbnails') else "No avatar"
            print(f"  - {v.get('title')[:40]} | Channel: {v.get('author')} | Avatar URL: {avatar}")
except Exception as e:
    print(f"Error fetching search: {e}")
