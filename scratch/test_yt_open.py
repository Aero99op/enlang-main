import urllib.request
import json
import re

# Test oEmbed
try:
    res = urllib.request.urlopen("https://www.youtube.com/oembed?url=https://www.youtube.com/watch?v=dQw4w9WgXcQ&format=json")
    data = json.loads(res.read().decode('utf-8'))
    print("oEmbed Success! Title:", data.get('title'), "Author:", data.get('author_name'))
except Exception as e:
    print("oEmbed Error:", e)

# Test YouTube Autocomplete / Suggestions
try:
    res = urllib.request.urlopen("https://suggestqueries.google.com/complete/search?client=firefox&ds=yt&q=spring+boot")
    data = json.loads(res.read().decode('utf-8'))
    print("Autocomplete Success! Suggestions:", data[1][:5])
except Exception as e:
    print("Autocomplete Error:", e)
