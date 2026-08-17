import urllib.request
import json
import re

def test_invidious_search(q):
    instances = [
        "https://invidious.privacydev.net",
        "https://vid.puffyan.us",
        "https://inv.tux.pizza",
        "https://invidious.nerdvpn.de",
        "https://invidious.io.lol"
    ]
    for inst in instances:
        try:
            url = f"{inst}/api/v1/search?q={urllib.parse.quote(q)}&type=video"
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req, timeout=5) as res:
                data = json.loads(res.read().decode())
                print(f"SUCCESS with {inst}! Found {len(data)} results.")
                for v in data[:3]:
                    print(f"  - {v.get('title')} | {v.get('author')} | ID: {v.get('videoId')}")
                return data
        except Exception as e:
            print(f"Failed {inst}: {e}")
    return None

def test_youtube_scrape(q):
    try:
        url = f"https://www.youtube.com/results?search_query={urllib.parse.quote(q)}"
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'})
        with urllib.request.urlopen(req, timeout=5) as res:
            html = res.read().decode()
            match = re.search(r'var ytInitialData = ({.*?});</script>', html)
            if match:
                data = json.loads(match.group(1))
                print("SUCCESS scraped YouTube ytInitialData directly!")
                # extract video items
                contents = data['contents']['twoColumnSearchResultsRenderer']['primaryContents']['sectionListRenderer']['contents']
                videos = []
                for sec in contents:
                    if 'itemSectionRenderer' in sec:
                        for item in sec['itemSectionRenderer']['contents']:
                            if 'videoRenderer' in item:
                                vr = item['videoRenderer']
                                vid = vr['videoId']
                                title = vr['title']['runs'][0]['text']
                                author = vr['ownerText']['runs'][0]['text']
                                length = vr.get('lengthText', {}).get('simpleText', '10:00')
                                views = vr.get('viewCountText', {}).get('simpleText', '100K views')
                                videos.append({'id': vid, 'title': title, 'channel': author, 'duration': length, 'views': views})
                print(f"Extracted {len(videos)} live YouTube videos for query '{q}':")
                for v in videos[:5]:
                    print(f"  - [{v['id']}] {v['title']} ({v['channel']}) - {v['duration']}")
                return videos
    except Exception as e:
        print("YouTube direct scrape error:", e)
    return None

print("--- Testing Live YouTube Search for 'snax gaming' ---")
res = test_youtube_scrape("snax gaming")
if not res:
    test_invidious_search("snax gaming")
