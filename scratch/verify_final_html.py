with open("youtube.html", "r", encoding="utf-8") as f:
    html = f.read()

print("HTML size:", len(html))
print("Has btn-theme listener:", "btn-theme" in html)
print("Has ch-1 chapter listener:", "ch-1" in html)
print("Has btn-player-play listener:", "btn-player-play" in html)
print("Has openWatchPage function:", "function openWatchPage" in html)
print("Has renderGrid function:", "function renderGrid" in html)
print("Has postComment function:", "function postComment" in html)
print("HTML ends with </html>:", html.strip().endswith("</html>"))
