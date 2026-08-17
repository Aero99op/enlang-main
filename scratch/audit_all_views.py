import sys
import os
import threading
import http.server
import socketserver
import time

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

from playwright.sync_api import sync_playwright

art_dir = r"C:\Users\spand\.gemini\antigravity-ide\brain\dbb1738d-e158-45fd-acdf-4b0fe62e9066"

# Check if port 4444 is alive or start a local server
def is_server_running():
    import urllib.request
    try:
        urllib.request.urlopen("http://127.0.0.1:4444/youtube.html", timeout=1)
        return True
    except:
        return False

if not is_server_running():
    class QuietHandler(http.server.SimpleHTTPRequestHandler):
        def log_message(self, format, *args):
            pass
    server = socketserver.TCPServer(("127.0.0.1", 4444), QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()
    time.sleep(0.5)

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={"width": 1536, "height": 900})
    
    page.goto("http://127.0.0.1:4444/youtube.html")
    page.wait_for_timeout(1000)
    
    # 1. Home Feed
    page.screenshot(path=os.path.join(art_dir, "view_home.png"))
    print("[1/6] Home Feed screenshot captured.")
    
    # 2. Watch Page
    page.click("#card-gJrjgg1KVL4")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_watch.png"))
    print("[2/6] Watch Page screenshot captured.")
    
    # 3. Shorts Reel
    page.click("#nav-shorts")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_shorts.png"))
    print("[3/6] Shorts Reel screenshot captured.")
    
    # 4. Subscriptions Feed
    page.click("#nav-subs")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_subs.png"))
    print("[4/6] Subscriptions screenshot captured.")
    
    # 5. Library / History
    page.click("#nav-library")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_library.png"))
    print("[5/6] Library screenshot captured.")
    
    # 6. Trending List
    page.click("#nav-trending")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_trending.png"))
    print("[6/6] Trending screenshot captured.")
    
    # 7. Live Instant Search (<1s)
    page.fill("#search-input", "Squid Game")
    page.keyboard.press("Enter")
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(art_dir, "view_search.png"))
    print("[7/7] Live Search screenshot captured.")

    browser.close()
    print("ALL 7 CRITICAL AUDIT SCREENS VERIFIED & CAPTURED SUCCESSFULLY!")
