with open("youtube.html", "r", encoding="utf-8") as f:
    content = f.read()

print("youtube.html length:", len(content))
print("Contains hideAllViews:", "hideAllViews" in content)
print("Contains openShortsReel:", "openShortsReel" in content)
print("Contains openSubsFeed:", "openSubsFeed" in content)
print("Contains openLibrary:", "openLibrary" in content)
print("Contains openTrending:", "openTrending" in content)
