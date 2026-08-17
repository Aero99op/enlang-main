import urllib.request

try:
    res = urllib.request.urlopen('http://127.0.0.1:4444/youtube.html')
    data = res.read()
    print("HTTP Status:", res.getcode(), "Length:", len(data))
except Exception as e:
    print("Error:", e)
