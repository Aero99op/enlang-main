import urllib.request

try:
    with urllib.request.urlopen("http://127.0.0.1:4444/youtube.html") as response:
        content = response.read().decode("utf-8")
        print("HTTP STATUS:", response.status)
        print("Content length:", len(content))
        print("Contains 'Spandan Prayas Patra':", "Spandan Prayas Patra" in content)
except Exception as e:
    print("Fetch error:", e)
