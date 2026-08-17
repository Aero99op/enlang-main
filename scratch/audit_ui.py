import sys
from playwright.sync_api import sync_playwright

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={"width": 1536, "height": 900})
    
    page.goto("http://127.0.0.1:4444/youtube.html")
    page.wait_for_timeout(1500)
    page.screenshot(path="scratch/audit_home.png", full_page=False)
    print("Screenshot 1: Home page captured.")
    
    # Click 1st video
    page.locator(".video-card").first.click()
    page.wait_for_timeout(1500)
    page.screenshot(path="scratch/audit_watch.png", full_page=False)
    print("Screenshot 2: Watch page captured.")
    
    # Click Shorts
    page.locator("#nav-shorts").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_shorts.png", full_page=False)
    print("Screenshot 3: Shorts page captured.")
    
    # Click Subscriptions
    page.locator("#nav-subs").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_subs.png", full_page=False)
    print("Screenshot 4: Subscriptions page captured.")
    
    # Click Library
    page.locator("#nav-library").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_library.png", full_page=False)
    print("Screenshot 5: Library page captured.")
    
    # Click Trending
    page.locator("#nav-trending").click()
    page.wait_for_timeout(1000)
    page.screenshot(path="scratch/audit_trending.png", full_page=False)
    print("Screenshot 6: Trending page captured.")
    
    browser.close()
    print("Audit screenshots complete.")
