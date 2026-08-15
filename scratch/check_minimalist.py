import urllib.request

try:
    res = urllib.request.urlopen('http://localhost:3333/minimalist.html')
    data = res.read()
    print("HTTP Status:", res.getcode(), "Length:", len(data))
except Exception as e:
    print("Error:", e)
