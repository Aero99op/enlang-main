with open("youtube.html", "r", encoding="utf-8") as f:
    content = f.read()

print("File size:", len(content))
print("searchLiveYouTube in content:", "searchLiveYouTube" in content)
if "<script>" in content:
    script_part = content.split("<script>")[1].split("</script>")[0]
    print("Script length:", len(script_part))
    print("First 300 chars of script:\n", script_part[:300])
    print("Last 300 chars of script:\n", script_part[-300:])
else:
    print("NO <script> tag found in youtube.html!")
