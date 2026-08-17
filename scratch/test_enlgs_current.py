import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

try:
    js = compile_enlgs_file("youtube.enlgs")
    print(f"SUCCESS! Length: {len(js)} bytes")
    print("Contains searchLiveYouTube:", "searchLiveYouTube" in js)
    print("Contains renderGrid:", "renderGrid" in js)
    print("Contains openWatchPage:", "openWatchPage" in js)
except Exception as e:
    import traceback
    traceback.print_exc()
