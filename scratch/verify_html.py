with open("youtube.html", "r", encoding="utf-8") as f:
    html = f.read()

print("HTML size:", len(html))
print("Has <script>:", "<script>" in html)
print("Has <iframe>:", "<iframe" in html)
print("Has searchLiveYouTube:", "searchLiveYouTube" in html)
