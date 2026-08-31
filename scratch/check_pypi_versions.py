import urllib.request
import json

try:
    url = "https://pypi.org/pypi/enlang/json"
    req = urllib.request.Request(url, headers={"User-Agent": "enlang-checker"})
    with urllib.request.urlopen(req) as resp:
        data = json.loads(resp.read().decode("utf-8"))
        releases = sorted(list(data["releases"].keys()))
        print("Existing PyPI releases:", releases)
except Exception as e:
    print("Error querying PyPI:", e)
