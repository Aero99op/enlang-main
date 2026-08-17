const { chromium } = require('playwright');
const path = require('path');

async function run() {
    const browser = await chromium.launch({ headless: true });
    const context = await browser.newContext({ viewport: { width: 1536, height: 900 } });
    const page = await context.newPage();
    
    page.on('console', msg => console.log(`[Browser] ${msg.text()}`));
    
    await page.goto('http://127.0.0.1:4444/youtube.html');
    await page.waitForTimeout(1000);
    
    const artDir = 'C:\\Users\\spand\\.gemini\\antigravity-ide\\brain\\dbb1738d-e158-45fd-acdf-4b0fe62e9066';
    
    console.log('1. Capturing Home Feed...');
    await page.screenshot({ path: path.join(artDir, 'view_home.png') });
    
    console.log('2. Clicking Video Card to open Watch Page...');
    await page.click('#card-gJrjgg1KVL4');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_watch.png') });
    
    console.log('3. Clicking Shorts Nav...');
    await page.click('#nav-shorts');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_shorts.png') });
    
    console.log('4. Clicking Subscriptions Nav...');
    await page.click('#nav-subs');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_subs.png') });
    
    console.log('5. Clicking Library Nav...');
    await page.click('#nav-library');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_library.png') });
    
    console.log('6. Clicking Trending Nav...');
    await page.click('#nav-trending');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_trending.png') });
    
    console.log('7. Testing Live Search...');
    await page.fill('#search-input', 'Squid Game');
    await page.keyboard.press('Enter');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(artDir, 'view_search.png') });
    
    await browser.close();
    console.log('ALL VIEWS TESTED & CAPTURED OUTSIDE WORKSPACE!');
}

run().catch(err => {
    console.error(err);
    process.exit(1);
});
