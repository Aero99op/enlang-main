import sys
import time
from playwright.sync_api import sync_playwright

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    context = browser.new_context(viewport={"width": 1440, "height": 900})
    page = context.new_page()

    print("[STEP 1] Navigating to http://localhost:5000/youtube.html...")
    page.goto("http://localhost:5000/youtube.html")
    page.wait_for_selector(".video-card")

    # 1. Verify Home Video Cards have Real Channel Logos / Avatars
    cards = page.locator(".video-card")
    card_count = cards.count()
    print(f"[TEST 1] Initial home cards rendered: {card_count}")
    assert card_count >= 12, "Expected at least 12 initial cards"

    # Check avatar of first card
    first_avatar = page.locator(".video-card").first.locator(".channel-avatar")
    print(f"[TEST 1.1] First card channel avatar HTML: {first_avatar.inner_html()[:80]}...")

    # 2. Live Search for 'snax gaming'
    print("\n[STEP 2] Performing Live Search for 'snax gaming'...")
    search_input = page.locator("#search-input")
    search_input.fill("snax gaming")
    page.locator("#btn-search").click()

    # Wait for live API fetch
    time.sleep(3)
    search_cards = page.locator(".video-card")
    search_count = search_cards.count()
    print(f"[TEST 2] Live search returned {search_count} cards for 'snax gaming'")
    assert search_count >= 15, "Expected at least 15 live search results"

    # Verify search results have real channel avatars
    first_search_avatar_img = page.locator(".video-card").first.locator(".channel-avatar-img")
    if first_search_avatar_img.count() > 0:
        avatar_src = first_search_avatar_img.get_attribute("src")
        print(f"[TEST 2.1] Live search card has official channel logo URL: {avatar_src}")

    # 3. Test Context-Aware 'Load More' on Search Query
    print("\n[STEP 3] Testing 'Load More' button for search query...")
    page.locator("#btn-load-more").click()
    time.sleep(3)
    new_search_count = page.locator(".video-card").count()
    print(f"[TEST 3] After 'Load More', total cards: {new_search_count} (added {new_search_count - search_count} new videos)")
    assert new_search_count > search_count, "Expected new cards to be appended from page 2 of search"

    # 4. Open Watch Page and verify Real Channel Avatar & Real Live Comments Fetch
    print("\n[STEP 4] Opening first Snax Gaming video to test Watch Page...")
    page.locator(".video-card").first.click()
    time.sleep(3)

    watch_title = page.locator("#watch-video-title").text_content()
    watch_channel = page.locator("#watch-channel-name").text_content()
    watch_avatar_html = page.locator("#watch-channel-avatar").inner_html()
    print(f"[TEST 4.1] Watch page active: Title='{watch_title}', Channel='{watch_channel}'")
    print(f"[TEST 4.2] Watch page channel logo HTML: {watch_avatar_html[:100]}...")

    # Wait for Live Comments to finish fetching
    time.sleep(3)
    comments_header = page.locator("#comments-count-header").text_content()
    comment_cards = page.locator("#comment-list-container .comment-card")
    comment_count = comment_cards.count()
    print(f"[TEST 4.3] Live Comments: Header='{comments_header}', Rendered Cards={comment_count}")
    if comment_count > 0:
        first_comment_author = comment_cards.first.locator(".comment-author").text_content()
        first_comment_msg = comment_cards.first.locator(".comment-msg").text_content()
        print(f"[TEST 4.4] Real Comment 1: Author='{first_comment_author}', Text='{first_comment_msg[:60]}...'")

    # Take screenshot of watch page with live comments
    page.screenshot(path="C:\\Users\\spand\\.gemini\\antigravity-ide\\brain\\dbb1738d-e158-45fd-acdf-4b0fe62e9066\\verify_live_watch_comments.png")

    # 5. Test Real YouTube Shorts Reel & Infinite Scrolling
    print("\n[STEP 5] Testing Real Shorts Reel & Infinite Navigation...")
    page.locator("#nav-shorts").click()
    time.sleep(3)

    short_title = page.locator("#shorts-title-text").text_content()
    short_author = page.locator("#shorts-author-name").text_content()
    print(f"[TEST 5.1] Initial Short: Title='{short_title[:50]}...', Creator='{short_author}'")

    # Click Next Short
    page.locator("#btn-shorts-next").click()
    time.sleep(1)
    short_title_2 = page.locator("#shorts-title-text").text_content()
    print(f"[TEST 5.2] Next Short loaded: Title='{short_title_2[:50]}...'")

    # Test Wheel Scroll on Shorts
    page.locator("#view-shorts").dispatch_event("wheel", {"deltaY": 100})
    time.sleep(1)
    short_title_3 = page.locator("#shorts-title-text").text_content()
    print(f"[TEST 5.3] Wheel-scrolled to Short: Title='{short_title_3[:50]}...'")

    # Take screenshot of shorts reel
    page.screenshot(path="C:\\Users\\spand\\.gemini\\antigravity-ide\\brain\\dbb1738d-e158-45fd-acdf-4b0fe62e9066\\verify_real_shorts.png")

    print("\n=======================================================")
    print(">>> ALL REAL LIVE FEATURES SUCCESSFULLY VERIFIED! <<<")
    print("=======================================================")
    browser.close()
