import sys
if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

from playwright.sync_api import sync_playwright

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={"width": 1536, "height": 900})
    page.on("console", lambda msg: print(f"[Browser Console] {msg.type}: {msg.text}"))
    page.on("pageerror", lambda err: print(f"[Browser Error] {err}"))
    
    page.goto("http://127.0.0.1:4444/youtube.html")
    page.wait_for_timeout(1000)
    
    # 1. Test clicking a video card
    print("Testing click on 1st video card...")
    card = page.locator("#card-gJrjgg1KVL4")
    if card.count() > 0:
        card.click()
        page.wait_for_timeout(1500)
        page.screenshot(path="scratch/audit_watch.png")
        print("Watch page screenshot taken.")
    else:
        print("Card #card-gJrjgg1KVL4 not found!")
        
    # 2. Test clicking Shorts nav
    print("Testing click on nav-shorts...")
    page.locator("#nav-shorts").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_shorts.png")
    print("Shorts screenshot taken.")

    # 3. Test clicking Subscriptions nav
    print("Testing click on nav-subs...")
    page.locator("#nav-subs").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_subs.png")
    print("Subs screenshot taken.")

    # 4. Test clicking Library nav
    print("Testing click on nav-library...")
    page.locator("#nav-library").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_library.png")
    print("Library screenshot taken.")

    # 5. Test clicking Trending nav
    print("Testing click on nav-trending...")
    page.locator("#nav-trending").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_trending.png")
    print("Trending screenshot taken.")

    # 6. Test search typing
    print("Testing search input...")
    page.fill("#search-input", "Squid Game")
    page.keyboard.press("Enter")
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_search.png")
    print("Search screenshot taken.")

    browser.close()
    print("All browser tests completed successfully!")
