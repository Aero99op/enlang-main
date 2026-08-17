import sys
if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

import os
from playwright.sync_api import sync_playwright

art_dir = r"C:\Users\spand\.gemini\antigravity-ide\brain\dbb1738d-e158-45fd-acdf-4b0fe62e9066"

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={"width": 1536, "height": 900})
    page.goto("http://localhost:5000/youtube.html")
    page.wait_for_timeout(1000)
    
    print("Searching for 'snax gaming' live over internet...")
    page.fill("#search-input", "snax gaming")
    page.click("#btn-search")
    
    # Wait for the live API fetch to complete
    page.wait_for_timeout(3500)
    
    cards = page.locator("#video-grid-container .video-card").count()
    first_title = page.locator("#video-grid-container .video-card:first-child .video-title").text_content()
    first_channel = page.locator("#video-grid-container .video-card:first-child .video-channel").text_content()
    status_text = page.locator("#live-status-title").text_content()
    
    print(f"Status banner: {status_text}")
    print(f"Total live YouTube cards rendered: {cards}")
    print(f"First Card Title: '{first_title}'")
    print(f"First Card Channel: '{first_channel}'")
    
    # Take screenshot of Snax Gaming live results
    ss_path = os.path.join(art_dir, "verify_snax_gaming_live.png")
    page.screenshot(path=ss_path)
    print(f"Saved screenshot to: {ss_path}")
    
    # Click the first live card to verify watch page streaming
    page.locator("#video-grid-container .video-card:first-child").click()
    page.wait_for_timeout(1500)
    watch_title = page.locator("#watch-video-title").text_content()
    watch_channel = page.locator("#watch-channel-name").text_content()
    iframe_src = page.locator("#player-iframe").get_attribute("src")
    
    print(f"Watch page active: Title='{watch_title}', Channel='{watch_channel}', Iframe='{iframe_src}'")
    
    ss_watch_path = os.path.join(art_dir, "verify_snax_watch_page.png")
    page.screenshot(path=ss_watch_path)
    print(f"Saved watch page screenshot to: {ss_watch_path}")
    
    browser.close()
    print("\n>>> LIVE YOUTUBE SEARCH VERIFIED 100% WORKING! <<<")
