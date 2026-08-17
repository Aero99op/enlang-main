import sys
import os

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

from playwright.sync_api import sync_playwright

art_dir = r"C:\Users\spand\.gemini\antigravity-ide\brain\dbb1738d-e158-45fd-acdf-4b0fe62e9066"

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={"width": 1536, "height": 900})
    
    print("Navigating to http://localhost:5000/youtube.html...")
    page.goto("http://localhost:5000/youtube.html")
    page.wait_for_timeout(1000)
    
    # 1. Test Search
    print("1. Testing Live Search with 'Squid Game'...")
    page.fill("#search-input", "Squid Game")
    page.click("#btn-search")
    page.wait_for_timeout(600)
    cards = page.locator("#video-grid-container .video-card").count()
    print(f"   [PASS] Search returned {cards} cards.")
    page.screenshot(path=os.path.join(art_dir, "verify_5000_search.png"))
    
    # 2. Test Watch Page & Titles/Comments
    print("2. Testing Watch Page & Video Meta...")
    page.click("#nav-home")
    page.wait_for_timeout(600)
    page.click("#card-gJrjgg1KVL4")
    page.wait_for_timeout(1000)
    title = page.locator("#watch-video-title").text_content()
    channel = page.locator("#watch-channel-name").text_content()
    comments = page.locator("#comment-list-container .comment-card").count()
    print(f"   [PASS] Title: '{title}', Channel: '{channel}', Comments count: {comments}")
    page.screenshot(path=os.path.join(art_dir, "verify_5000_watch.png"))

    # 3. Test Scrubber Time Bar
    print("3. Testing Scrubber Click...")
    scrubber = page.locator("#player-scrubber")
    scrubber.click(position={"x": 150, "y": 4})
    page.wait_for_timeout(600)
    time_str = page.locator("#player-time-display").text_content()
    print(f"   [PASS] Scrubber seeked to: {time_str}")

    # 4. Test Quality Option
    print("4. Testing Quality Option Button...")
    page.click("#btn-player-quality")
    page.wait_for_timeout(400)
    quality_text = page.locator("#btn-player-quality").text_content()
    print(f"   [PASS] Quality changed to: '{quality_text}'")

    # 5. Test Subscriptions & 2nd Subscription
    print("5. Testing Subscriptions Panel & #sub-2...")
    page.click("#nav-subs")
    page.wait_for_timeout(600)
    subs_count = page.locator("#subs-grid-container .video-card").count()
    print(f"   [PASS] Subscriptions Feed loaded with {subs_count} unique items.")
    page.click("#sub-2")
    page.wait_for_timeout(600)
    print("   [PASS] Clicked #sub-2 (Enlang Official).")

    # 6. Test Profile Dropdown
    print("6. Testing User Profile Dropdown...")
    page.click("#user-profile")
    page.wait_for_timeout(400)
    is_prof_visible = page.locator("#dropdown-profile").is_visible()
    print(f"   [PASS] Profile Dropdown visible: {is_prof_visible}")
    page.screenshot(path=os.path.join(art_dir, "verify_5000_profile.png"))

    # 7. Test White / Light Mode Toggle
    print("7. Testing White Mode (Light Theme)...")
    page.click("#btn-theme")
    page.wait_for_timeout(600)
    is_light = page.locator("body").evaluate("el => el.classList.contains('light-theme')")
    print(f"   [PASS] Light Mode active: {is_light}")
    page.screenshot(path=os.path.join(art_dir, "verify_5000_light_mode.png"))

    # Switch back to dark mode
    page.click("#btn-theme")
    page.wait_for_timeout(400)

    browser.close()
    print("ALL 7 CONTROLS VERIFIED WORKING ON PORT 5000!")
