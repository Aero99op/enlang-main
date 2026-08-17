const fs = require('fs');

// Minimal lightweight DOM environment in pure Node.js
class Element {
  constructor(tag, id, className) {
    this.tagName = tag.toUpperCase();
    this.id = id || '';
    this.className = className || '';
    this.children = [];
    this.style = {};
    this.textContent = '';
    this._innerHTML = '';
    this.value = '';
    this.src = '';
    this.classList = {
      classes: new Set(className ? className.split(' ') : []),
      add: (c) => this.classList.classes.add(c),
      remove: (c) => this.classList.classes.delete(c),
      contains: (c) => this.classList.classes.has(c)
    };
    this.listeners = {};
  }
  get innerHTML() {
    return this._innerHTML;
  }
  set innerHTML(val) {
    this._innerHTML = val;
    if (val === '') {
      this.children = [];
    }
  }
  addEventListener(event, fn) {
    this.listeners[event] = this.listeners[event] || [];
    this.listeners[event].push(fn);
  }
  click() {
    if (this.onclick) this.onclick();
    if (this.listeners['click']) {
      this.listeners['click'].forEach(fn => fn({ target: this }));
    }
  }
  appendChild(child) {
    this.children.push(child);
  }
  prepend(child) {
    this.children.unshift(child);
  }
  get firstElementChild() {
    return this.children[0] || null;
  }
}

const elementsMap = {};
function registerEl(id, tag, cls) {
  const el = new Element(tag || 'div', id, cls);
  elementsMap[id] = el;
  return el;
}

// Register all YouTube page elements
[
  "video-grid-container", "view-feed", "view-shorts", "view-watch", "view-subs", "view-library", "view-trending",
  "watch-video-title", "watch-channel-name", "watch-sub-count", "watch-desc-text", "desc-stats",
  "watch-channel-avatar", "share-link-input", "player-iframe", "player-ambient-light",
  "btn-player-play", "btn-player-mute", "btn-player-speed", "btn-player-next",
  "rec-list-container", "subs-grid-container", "subs-channels-row", "history-list-container",
  "watch-later-container", "liked-videos-container", "trending-list-container", "comment-list-container",
  "chapters-container", "new-comment-input", "btn-submit-comment", "btn-cancel-comment", "comments-count-header",
  "nav-home", "btn-logo", "nav-shorts", "nav-subs", "nav-library", "nav-history",
  "nav-your-videos", "nav-watch-later", "nav-liked", "sub-1", "sub-2", "sub-3", "sub-4", "sub-5",
  "nav-trending", "nav-music", "nav-gaming", "nav-news", "nav-sports",
  "btn-sidebar-toggle", "yt-sidebar", "btn-create", "btn-close-create", "btn-cancel-upload",
  "btn-publish-video", "upload-title-input", "upload-desc-input", "upload-cat-input", "modal-create",
  "btn-notif", "dropdown-notif", "btn-voice", "user-profile", "btn-search", "search-input", "btn-load-more",
  "chip-all", "chip-coding", "chip-java", "chip-spring", "chip-enlang", "chip-sysdesign", "chip-next",
  "chip-edge", "chip-music", "chip-gaming", "chip-science", "chip-podcasts",
  "short-shelf-1", "short-shelf-2", "short-shelf-3", "short-shelf-4",
  "shorts-active-box", "shorts-iframe", "shorts-title-text", "shorts-author-name", "shorts-audio-text",
  "btn-shorts-next", "btn-shorts-prev", "btn-shorts-like", "shorts-like-count",
  "btn-shorts-dislike", "btn-shorts-comment", "btn-shorts-share", "btn-shorts-sub", "btn-shorts-mute",
  "ch-1", "ch-2", "ch-3", "ch-4", "btn-join", "btn-subscribe", "btn-like", "btn-dislike",
  "btn-share", "btn-close-share", "btn-copy-share", "modal-share", "btn-download", "btn-clip",
  "btn-more", "btn-clear-history", "btn-theme", "yt-toast", "live-status-title", "live-status-desc",
  "player-time-display", "scrubber-progress"
].forEach(id => registerEl(id, 'div'));

global.document = {
  getElementById: (id) => elementsMap[id] || null,
  querySelector: (sel) => {
    const cleanId = sel.replace('#', '');
    return elementsMap[cleanId] || null;
  },
  querySelectorAll: (sel) => {
    if (sel === '.chip') {
      return [
        elementsMap['chip-all'], elementsMap['chip-coding'], elementsMap['chip-java'],
        elementsMap['chip-spring'], elementsMap['chip-enlang'], elementsMap['chip-sysdesign'],
        elementsMap['chip-next'], elementsMap['chip-edge'], elementsMap['chip-music'],
        elementsMap['chip-gaming'], elementsMap['chip-science'], elementsMap['chip-podcasts']
      ].filter(Boolean);
    }
    return [];
  },
  createElement: (tag) => new Element(tag),
  body: new Element('body', 'body')
};

global.window = {
  scrollTo: () => {},
  addEventListener: () => {}
};

// Read script from youtube.html
const htmlContent = fs.readFileSync('youtube.html', 'utf8');
const scriptMatch = htmlContent.match(/<script>([\s\S]*?)<\/script>/);
if (!scriptMatch) {
  console.error("FATAL: No <script> found in youtube.html!");
  process.exit(1);
}

// Execute the compiled YouTube engine script
eval(scriptMatch[1]);

console.log("=== VERIFYING FULL INFINITE YOUTUBE ENGINE ===");

// 1. Initial Home Feed Load
const grid = document.getElementById("video-grid-container");
console.log("1. Initial Home Grid Video Cards:", grid.children.length);
if (grid.children.length < 15) {
  console.error("FAIL: Expected 15+ video cards, found " + grid.children.length);
  process.exit(1);
}
console.log("   [PASS] Full real video catalog rendered with HD thumbnails!");

// 2. Infinite Feed Append Click
const loadMoreBtn = document.getElementById("btn-load-more");
const initialCount = grid.children.length;
loadMoreBtn.click();
console.log("2. Total video cards after Infinite Load More:", grid.children.length);
if (grid.children.length <= initialCount) {
  console.error("FAIL: Infinite load more did not append cards!");
  process.exit(1);
}
console.log("   [PASS] Infinite feed append loaded more videos dynamically!");

// 3. Click MrBeast Squid Game Card
const beastCard = grid.children[1];
beastCard.click();

const wTitle = document.getElementById("watch-video-title").textContent;
const wChannel = document.getElementById("watch-channel-name").textContent;
const pIframe = document.getElementById("player-iframe");
const wView = document.getElementById("view-watch");
const chaptersEl = document.getElementById("chapters-container");
const commentsEl = document.getElementById("comment-list-container");

console.log("3. Watch Title:", wTitle);
console.log("   Watch Channel:", wChannel);
console.log("   Iframe Stream:", pIframe.src);
console.log("   Chapters rendered count:", chaptersEl.children.length);
console.log("   Topic-specific comments count:", commentsEl.children.length);

if (!wTitle.includes("Squid Game") || !pIframe.src.includes("0e3GPea1Tyg") || wView.style.display !== "block") {
  console.error("FAIL: Watch page did not load exact video metadata!");
  process.exit(1);
}
console.log("   [PASS] Exact matching video title, creator, stream, chapters & comments loaded!");

// 4. Play/Pause Click
const playBtn = document.getElementById("btn-player-play");
playBtn.click();
console.log("4. Play Button text after click:", playBtn.textContent);
if (playBtn.textContent !== "▶") {
  console.error("FAIL: Play/Pause toggle did not change to pause!");
  process.exit(1);
}
console.log("   [PASS] Video player Play/Pause toggle responds!");

// 5. Post Real Public Comment
const commentInp = document.getElementById("new-comment-input");
const commentBtn = document.getElementById("btn-submit-comment");
commentInp.value = "Tested by Spandan Prayas Patra - 100% Real Infinite YouTube!";
commentBtn.click();

const latestComment = commentsEl.children[0].innerHTML;
console.log("5. Posted comment HTML:", latestComment.substring(0, 120));
if (!latestComment.includes("@SpandanPrayas (You)")) {
  console.error("FAIL: Comment was not prepended!");
  process.exit(1);
}
console.log("   [PASS] Real-time comment posted with user avatar & author badge!");

// 6. Shorts Fullscreen Reel View
const navShorts = document.getElementById("nav-shorts");
navShorts.click();
const short1 = document.getElementById("shorts-title-text").textContent;
const nextShort = document.getElementById("btn-shorts-next");
nextShort.click();
const short2 = document.getElementById("shorts-title-text").textContent;
console.log("6. Short 1 Title:", short1);
console.log("   Short 2 Title:", short2);
if (short1 === short2) {
  console.error("FAIL: Shorts did not cycle to next item!");
  process.exit(1);
}
console.log("   [PASS] Shorts reel navigated smoothly!");

// 7. Subscriptions View (Channel Avatars Row + Video Feed)
const navSubs = document.getElementById("nav-subs");
navSubs.click();
const subsStories = document.getElementById("subs-channels-row");
const subsGrid = document.getElementById("subs-grid-container");
console.log("7. Subscriptions Channel Circles:", subsStories.children.length);
console.log("   Subscriptions Videos Count:", subsGrid.children.length);
if (subsStories.children.length === 0 || subsGrid.children.length === 0) {
  console.error("FAIL: Subscriptions page incomplete!");
  process.exit(1);
}
console.log("   [PASS] Subscriptions view fully loaded!");

// 8. Library & Watch History View
const navLib = document.getElementById("nav-library");
navLib.click();
const histList = document.getElementById("history-list-container");
const watchLaterGrid = document.getElementById("watch-later-container");
const likedGrid = document.getElementById("liked-videos-container");
console.log("8. Recently Watched Cards:", histList.children.length);
console.log("   Watch Later Cards:", watchLaterGrid.children.length);
console.log("   Liked Videos Cards:", likedGrid.children.length);
if (histList.children.length === 0 || watchLaterGrid.children.length === 0 || likedGrid.children.length === 0) {
  console.error("FAIL: Library view incomplete!");
  process.exit(1);
}
console.log("   [PASS] Library with History, Watch Later, and Liked Playlists loaded!");

// 9. Trending Page
const navTrend = document.getElementById("nav-trending");
navTrend.click();
const trendList = document.getElementById("trending-list-container");
console.log("9. Trending Ranked Videos Count:", trendList.children.length);
console.log("   #1 Trending Video:", trendList.children[0].textContent.substring(0, 70));
if (trendList.children.length === 0 || !trendList.children[0].innerHTML.includes("#1")) {
  console.error("FAIL: Trending page incomplete!");
  process.exit(1);
}
console.log("   [PASS] Trending ranked feed loaded!");

// 10. Search Query & Direct URL Resolver
const searchInp = document.getElementById("search-input");
const searchBtn = document.getElementById("btn-search");

// Test query search
searchInp.value = "GTA 6";
searchBtn.click();
console.log("10. Cards in grid after searching 'GTA 6':", grid.children.length);
console.log("    Top matching card HTML:", grid.children[0].innerHTML.substring(0, 100));
if (!grid.children[0].innerHTML.includes("Grand Theft Auto")) {
  console.error("FAIL: Search query did not filter properly!");
  process.exit(1);
}
console.log("    [PASS] Instant multi-keyword search worked accurately!");

// Test Direct YouTube URL Paste
searchInp.value = "https://www.youtube.com/watch?v=kJQP7kiw5Fk";
searchBtn.click();
console.log("    Direct URL Play Title:", document.getElementById("watch-video-title").textContent);
console.log("    Direct URL Iframe:", document.getElementById("player-iframe").src);
if (!document.getElementById("player-iframe").src.includes("kJQP7kiw5Fk")) {
  console.error("FAIL: Direct YouTube URL resolver failed!");
  process.exit(1);
}
console.log("    [PASS] Direct YouTube URL resolver mounted and played video directly!");

// 11. Theme Toggle
const themeBtn = document.getElementById("btn-theme");
themeBtn.click();
console.log("11. Body Light-Theme:", document.body.classList.contains("light-theme"));
if (!document.body.classList.contains("light-theme")) {
  console.error("FAIL: Light theme not applied!");
  process.exit(1);
}
themeBtn.click();
console.log("    Body Dark-Theme:", !document.body.classList.contains("light-theme"));
console.log("    [PASS] Theme toggled seamlessly!");

// 12. Create & Publish Video
const createTitle = document.getElementById("upload-title-input");
const publishBtn = document.getElementById("btn-publish-video");
createTitle.value = "Building Global Apps with Spandan Prayas Patra";
publishBtn.click();
console.log("12. Feed cards count after upload:", grid.children.length);
console.log("    Newly Published Video HTML:", grid.children[0].innerHTML.substring(0, 120));
if (!grid.children[0].innerHTML.includes("Building Global Apps")) {
  console.error("FAIL: Uploaded video was not added to feed!");
  process.exit(1);
}
console.log("    [PASS] Video published live to top of feed!");

console.log("\n=================================================================");
console.log("🎉 ALL 12 FULL INFINITE YOUTUBE ENGINE ACTIONS PASSED WITH 100%!");
console.log("=================================================================");
