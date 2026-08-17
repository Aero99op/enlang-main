
let isPlaying = true;

let isMuted = false;

let isSubscribed = false;

let isLiked = false;

let currentVideoId = "gJrjgg1KVL4";

let currentShortIndex = 0;

let totalComments = 0;

let playSeconds = 258;

let totalVideoSeconds = 4282;

let playbackSpeed = "1x";

let isShortsLiked = false;

let currentCategoryFilter = "All";

let qualityIndex = 0;

let currentSearchQuery = "";

let currentFeedPage = 1;

let currentShortsPage = 1;

let isFetchingShorts = false;

console.log("▶ Initializing 1:1 Live YouTube Engine with Real-Time Live Comments & Avatars (.enlgs)...");

const homeCatalog = [ { id : "gJrjgg1KVL4" , title : "Spring Boot 3 Full Course 2025 - Beginner to Pro" , channel : "Spandan Prayas Patra" , subs : "128K subscribers" , views : "1,039,987 views • Premiered recently" , desc : "In this deep dive tutorial, Spandan Prayas Patra walks through architecting mission-critical Java 21 backend microservices, configuring HikariCP connection pools for Oracle & MySQL, and applying strict ACID transaction management." , category : "Java Backend" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "1:11:22" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } , { id : "0e3GPea1Tyg" , title : "$456,000 Squid Game In Real Life!" , channel : "MrBeast" , subs : "310M subscribers" , views : "630,240,119 views • 2 years ago" , desc : "456 people compete for $456,000 in real life recreation of Squid Game! Red Light Green Light, Dalgona, Tug of War, Marbles, Glass Bridge, and the Final Battle." , category : "Science & Tech" , ambient : "rgba(239, 68, 68, 0.35)" , duration : "25:41" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } , { id : "dQw4w9WgXcQ" , title : "Rick Astley - Never Gonna Give You Up (Official Music Video 4K Remaster)" , channel : "Rick Astley" , subs : "3.4M subscribers" , views : "1,520,840,112 views • 14 years ago" , desc : "The official video for 'Never Gonna Give You Up' by Rick Astley. 4K Remastered with studio high definition audio." , category : "Lo-Fi Music" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "03:33" , avatar : "https://yt3.ggpht.com/G5qB0H5gZ0W5n5O_s9l8W4Q7k5U7s4P8=s512-c-k-c0x00ffffff-no-rj" } , { id : "8aGhZQkoFbQ" , title : "Next.js 15 & React 19 Full Stack Masterclass (App Router, Server Actions)" , channel : "JavaScript Mastery" , subs : "1.1M subscribers" , views : "489,420 views • 3 weeks ago" , desc : "Master full-stack Next.js with Server Actions, Edge Middleware, and Cloudflare Deployments with zero 500 errors." , category : "Next.js" , ambient : "rgba(16, 185, 129, 0.35)" , duration : "2:45:10" , avatar : "https://yt3.ggpht.com/wg1TITEoPfxvBGfzu-mm1mVmEBoTU4m9Lk0dtoqY4AXjS5ndQm92RrGQMgL2xKTcq0JyNxW9=s512-c-k-c0x00ffffff-no-rj" } , { id : "pTJJsmejUOQ" , title : "Flutter 3.24 & Dart Full Course for Cross-Platform Mobile Apps" , channel : "FreeCodeCamp" , subs : "9.8M subscribers" , views : "1,420,150 views • 2 months ago" , desc : "Build cross-platform iOS & Android mobile applications with clean architecture and state management." , category : "Enlang Stack" , ambient : "rgba(20, 184, 166, 0.35)" , duration : "4:32:00" , avatar : "https://yt3.ggpht.com/ytc/AIdro_kX43X6XJ5n5mP3K-s7m5k=s512-c-k-c0x00ffffff-no-rj" } , { id : "L_LUpnjgPso" , title : "Grand Theft Auto VI Official Trailer 1 - 4K 60FPS" , channel : "Rockstar Games" , subs : "10.8M subscribers" , views : "215,900,000 views • 8 months ago" , desc : "Grand Theft Auto VI heads to the state of Leonida, home to the neon-soaked streets of Vice City." , category : "Gaming" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "01:31" , avatar : "https://yt3.ggpht.com/ytc/AIdro_n8z5R9K3m5J7s=s512-c-k-c0x00ffffff-no-rj" } , { id : "Q7AOvWpIVHU" , title : "Three.js 3D Web Graphics & WebGL Shaders Masterclass" , channel : "Creative Coding 3D" , subs : "340K subscribers" , views : "340,900 views • 5 days ago" , desc : "Shader art tutorial implementing dual plasma rings and procedural celestial textures in WebGL." , category : "Coding & AI" , ambient : "rgba(99, 102, 241, 0.35)" , duration : "21:15" , avatar : "https://yt3.ggpht.com/ytc/AIdro_j4K2L5m=s512-c-k-c0x00ffffff-no-rj" } , { id : "W6NZfCO5SIk" , title : "JavaScript Tutorial for Beginners: Learn JavaScript in 1 Hour [2025]" , channel : "Programming with Mosh" , subs : "4.1M subscribers" , views : "14,200,000 views • 4 years ago" , desc : "Watch this JavaScript tutorial for beginners to learn JavaScript basics in 1 hour. Get started with frontend and fullstack web development." , category : "Coding & AI" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "48:16" , avatar : "https://yt3.ggpht.com/tBEPr-KMMNBo7DaPhEOKWJyNxaumWg3auUmQKiWwiaTFpochCULNu_2UsKnHtrmKuDuJaG=s512-c-k-c0x00ffffff-no-rj" } , { id : "h4T_LlK1VE4" , title : "Glitter Bomb 4.0 vs. Porch Pirates" , channel : "Mark Rober" , subs : "56.4M subscribers" , views : "74,900,000 views • 2 years ago" , desc : "The engineering-heavy glitter bomb trap for package thieves featuring fart spray and 360-degree phone cameras." , category : "Science & Tech" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "22:30" , avatar : "https://yt3.ggpht.com/ytc/AIdro_n1k3L4m=s512-c-k-c0x00ffffff-no-rj" } , { id : "fJ9rUzIMcZQ" , title : "Queen - Bohemian Rhapsody (Official Video Remastered)" , channel : "Queen Official" , subs : "18.2M subscribers" , views : "1,740,290,000 views • 15 years ago" , desc : "The official music video for Queen's iconic Bohemian Rhapsody, remastered in HD." , category : "Lo-Fi Music" , ambient : "rgba(168, 85, 247, 0.35)" , duration : "05:59" , avatar : "https://yt3.ggpht.com/ytc/AIdro_l2K5M=s512-c-k-c0x00ffffff-no-rj" } , { id : "kXYiU_JCYtU" , title : "Linkin Park - Numb (Official Music Video 4K)" , channel : "Linkin Park" , subs : "21.4M subscribers" , views : "2,210,000,000 views • 17 years ago" , desc : "'Numb' by Linkin Park from the album Meteora. 4K Ultra High Definition remastered." , category : "Lo-Fi Music" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "03:07" , avatar : "https://yt3.ggpht.com/ytc/AIdro_p8K4m=s512-c-k-c0x00ffffff-no-rj" } , { id : "5qap5aO4i9A" , title : "lofi hip hop radio 📚 - beats to relax/study to" , channel : "Lofi Girl" , subs : "14.1M subscribers" , views : "148,400,000 views • Live 24/7" , desc : "Peaceful lofi hip hop beats to relax, study, code, and sleep to. Welcome to the official Lofi Girl stream." , category : "Lo-Fi Music" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "LIVE" , avatar : "https://yt3.ggpht.com/ytc/AIdro_lofi=s512-c-k-c0x00ffffff-no-rj" } ];

const subsCatalog = [ { id : "gJrjgg1KVL4" , title : "Java 21 Virtual Threads & Low Latency Concurrency" , channel : "Spandan Prayas Patra" , views : "412K views • 2 days ago" , duration : "48:15" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } , { id : "8aGhZQkoFbQ" , title : "Next.js 15 Server Actions & Cloudflare Workers 0ms Latency" , channel : "JavaScript Mastery" , views : "290K views • 4 days ago" , duration : "1:15:30" , avatar : "https://yt3.ggpht.com/wg1TITEoPfxvBGfzu-mm1mVmEBoTU4m9Lk0dtoqY4AXjS5ndQm92RrGQMgL2xKTcq0JyNxW9=s512-c-k-c0x00ffffff-no-rj" } , { id : "0e3GPea1Tyg" , title : "I Spent 7 Days Stranded In An Abandoned City" , channel : "MrBeast" , views : "89M views • 1 week ago" , duration : "22:10" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } , { id : "pTJJsmejUOQ" , title : "React Native 2025 Architecture Full Course" , channel : "FreeCodeCamp" , views : "820K views • 1 week ago" , duration : "3:40:00" , avatar : "https://yt3.ggpht.com/ytc/AIdro_kX43X6XJ5n5mP3K-s7m5k=s512-c-k-c0x00ffffff-no-rj" } ];

const historyCatalog = [ { id : "gJrjgg1KVL4" , title : "Spring Boot 3 Full Course 2025 - Beginner to Pro" , channel : "Spandan Prayas Patra" , views : "1.0M views" , time : "Watched 1 hour ago" , duration : "1:11:22" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } , { id : "8aGhZQkoFbQ" , title : "Next.js 15 & React 19 Full Stack Masterclass" , channel : "JavaScript Mastery" , views : "489K views" , time : "Watched 4 hours ago" , duration : "2:45:10" , avatar : "https://yt3.ggpht.com/wg1TITEoPfxvBGfzu-mm1mVmEBoTU4m9Lk0dtoqY4AXjS5ndQm92RrGQMgL2xKTcq0JyNxW9=s512-c-k-c0x00ffffff-no-rj" } , { id : "0e3GPea1Tyg" , title : "$456,000 Squid Game In Real Life!" , channel : "MrBeast" , views : "630M views" , time : "Watched last week" , duration : "25:41" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } ];

const watchLaterCatalog = [ { id : "8aGhZQkoFbQ" , title : "Next.js 15 & React 19 Full Stack Masterclass" , channel : "JavaScript Mastery" , views : "489K views" , duration : "2:45:10" , avatar : "https://yt3.ggpht.com/wg1TITEoPfxvBGfzu-mm1mVmEBoTU4m9Lk0dtoqY4AXjS5ndQm92RrGQMgL2xKTcq0JyNxW9=s512-c-k-c0x00ffffff-no-rj" } ];

const likedCatalog = [ { id : "0e3GPea1Tyg" , title : "$456,000 Squid Game In Real Life!" , channel : "MrBeast" , views : "630M views" , duration : "25:41" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } ];

const trendingCatalog = [ { rank : 1 , id : "0e3GPea1Tyg" , title : "$456,000 Squid Game In Real Life!" , channel : "MrBeast" , views : "630M views • Trending #1 on YouTube" , desc : "456 people compete for $456,000 in real life recreation of Squid Game!" , duration : "25:41" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } , { rank : 2 , id : "L_LUpnjgPso" , title : "Grand Theft Auto VI Official Trailer 1 - 4K 60FPS" , channel : "Rockstar Games" , views : "215M views • Trending #2 on YouTube" , desc : "Grand Theft Auto VI heads to the state of Leonida, home to the neon-soaked streets of Vice City." , duration : "01:31" , avatar : "https://yt3.ggpht.com/ytc/AIdro_n8z5R9K3m5J7s=s512-c-k-c0x00ffffff-no-rj" } , { rank : 3 , id : "gJrjgg1KVL4" , title : "Spring Boot 3 Full Course 2025 - Beginner to Pro" , channel : "Spandan Prayas Patra" , views : "1.0M views • Trending #3 on YouTube" , desc : "Complete guide to Java 21, Spring Boot 3, and high performance backend engineering." , duration : "1:11:22" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } ];

const dynamicShortsList = [ { title : "🔥 3 Insane Java 21 Performance Hacks you never knew existed! #Java #SpringBoot #Dev" , author : "@SpandanPrayas" , audio : "🎵 Spandan Original Audio • High Performance Remix" , views : "3.2M" , bg : "linear-gradient(180deg, #1e3a8a, #0f172a)" , vid : "gJrjgg1KVL4" } , { title : "⚡ $456,000 Squid Game Behind The Scenes Secret #MrBeast #Challenge" , author : "@MrBeast" , audio : "🎵 Squid Game Official Theme" , views : "45.1M" , bg : "linear-gradient(180deg, #991b1b, #450a0a)" , vid : "0e3GPea1Tyg" } , { title : "🎮 GTA 6 Vice City Graphics will blow your mind #GTA6 #Rockstar #Gaming" , author : "@RockstarGames" , audio : "🎵 Love Is A Long Road • Tom Petty" , views : "12.4M" , bg : "linear-gradient(180deg, #831843, #500724)" , vid : "L_LUpnjgPso" } ];

function showToast(message) {
  (document.getElementById("yt-toast") || document.querySelector("yt-toast")).textContent = message;
  (document.getElementById("yt-toast") || document.querySelector("yt-toast")).style.display = 'block';
  setTimeout(function() {
    (document.getElementById("yt-toast") || document.querySelector("yt-toast")).style.display = 'none';
  }, 3000);
}

function formatDuration(secs) {
  let m = Math.floor ( secs / 60 );
  let s = secs % 60;
  let mStr = m < 10 ? "0" + m : "" + m;
  let sStr = s < 10 ? "0" + s : "" + s;
  return mStr + ":" + sStr;
}

function sendPlayerCmd(funcName, args) {
  let ifr = document.getElementById ( "player-iframe" );
  if (ifr != null && ifr.contentWindow != null) {
    ifr.contentWindow.postMessage ( JSON.stringify ( { event : "command" , func : funcName , args : args || [ ] } ) , "*" );
  }
}

function sendShortsCmd(funcName, args) {
  let ifr = document.getElementById ( "shorts-iframe" );
  if (ifr != null && ifr.contentWindow != null) {
    ifr.contentWindow.postMessage ( JSON.stringify ( { event : "command" , func : funcName , args : args || [ ] } ) , "*" );
  }
}

function seekScrubber(evt) {
  let scrubberEl = document.getElementById ( "player-scrubber" );
  if (scrubberEl != null) {
    let rect = scrubberEl.getBoundingClientRect ( );
    let pos = ( evt.clientX - rect.left ) / rect.width;
    let clampedPos = Math.max ( 0 , Math.min ( 1 , pos ) );
    playSeconds = Math.floor ( clampedPos * totalVideoSeconds );
    let fillBar = document.getElementById ( "scrubber-progress" );
    if (fillBar != null) {
      fillBar.style.width = ( clampedPos * 100 ) + "%";
    }
    let curStr = formatDuration ( playSeconds );
    let totStr = formatDuration ( totalVideoSeconds );
    (document.getElementById("player-time-display") || document.querySelector("player-time-display")).textContent = curStr + " / " + totStr;
    sendPlayerCmd("seekTo" , [ playSeconds , true ]);
    showToast("⏩ Seeked to " + curStr);
  }
}

function seekToChapter(sec, label) {
  playSeconds = sec;
  sendPlayerCmd("seekTo" , [ sec , true ]);
  showToast("📍 Seeked to " + label);
}

function hideAllViews() {
  (document.getElementById("view-feed") || document.querySelector("view-feed")).style.display = 'none';
  (document.getElementById("view-shorts") || document.querySelector("view-shorts")).style.display = 'none';
  (document.getElementById("view-watch") || document.querySelector("view-watch")).style.display = 'none';
  (document.getElementById("view-subs") || document.querySelector("view-subs")).style.display = 'none';
  (document.getElementById("view-library") || document.querySelector("view-library")).style.display = 'none';
  (document.getElementById("view-trending") || document.querySelector("view-trending")).style.display = 'none';
  (document.getElementById("dropdown-profile") || document.querySelector("dropdown-profile")).style.display = 'none';
  (document.getElementById("dropdown-notif") || document.querySelector("dropdown-notif")).style.display = 'none';
}

function openHomeFeed() {
  currentSearchQuery = "";
  currentFeedPage = 1;
  hideAllViews();
  (document.getElementById("view-feed") || document.querySelector("view-feed")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  renderGrid(homeCatalog);
  (document.getElementById("live-status-title") || document.querySelector("live-status-title")).textContent = "🔴 YouTube Live Feed";
  (document.getElementById("live-status-desc") || document.querySelector("live-status-desc")).textContent = "Displaying verified live YouTube stream catalog • Infinite Scroll active";
  showToast("🏠 YouTube Home Feed");
}

function openShortsReel() {
  hideAllViews();
  (document.getElementById("view-shorts") || document.querySelector("view-shorts")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  fetchMoreRealShorts();
  renderActiveShort(0);
  showToast("⚡ YouTube Shorts Reel active");
}

function openSubsFeed() {
  hideAllViews();
  (document.getElementById("view-subs") || document.querySelector("view-subs")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  renderSubsGrid ( );
  showToast("📺 Subscriptions Feed");
}

function openLibrary() {
  hideAllViews();
  (document.getElementById("view-library") || document.querySelector("view-library")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  renderLibraryView ( );
  showToast("📁 Library & Watch History");
}

function openTrending() {
  hideAllViews();
  (document.getElementById("view-trending") || document.querySelector("view-trending")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  renderTrendingList ( );
  showToast("🔥 Trending on YouTube");
}

async function fetchMoreRealShorts() {
  if (isFetchingShorts == true) {
    return;
  }
  isFetchingShorts = true;
  let sUrl = "https://invidious.flokinet.to/api/v1/search?q=" + encodeURIComponent ( "#shorts viral trending" ) + "&page=" + currentShortsPage + "&type=video";
  try {
    let resp = await fetch ( sUrl );
    let dataList = await resp.json ( );
    if (dataList != null && dataList.length > 0) {
      for (const sItem of dataList) {
        let sId = sItem.videoId;
        let sTitle = sItem.title || "YouTube Short";
        let sAuthor = "@" + ( sItem.author ? sItem.author.split ( " " ) .join ( "" ) : "Creator" );
        let sViews = sItem.viewCount ? ( sItem.viewCount > 1000000 ? ( sItem.viewCount / 1000000 ) .toFixed ( 1 ) + "M" : ( sItem.viewCount / 1000 ) .toFixed ( 0 ) + "K" ) : "1.2M";
        let sBg = "linear-gradient(180deg, #1e1b4b, #0f172a)";
        dynamicShortsList.push ( { title : sTitle , author : sAuthor , audio : "🎵 Original Audio • Trending Sound" , views : sViews , bg : sBg , vid : sId } );
      }
      currentShortsPage = currentShortsPage + 1;
    }
  } catch (err) {
    console.log("Shorts fetch fallback active");
  }
  isFetchingShorts = false;
}

function renderActiveShort(idx) {
  if (idx >= dynamicShortsList.length - 2) {
    fetchMoreRealShorts();
  }
  if (idx < 0) {
    idx = 0;
  }
  if (idx >= dynamicShortsList.length) {
    idx = dynamicShortsList.length - 1;
  }
  currentShortIndex = idx;
  let s = dynamicShortsList [ idx ];
  if (s == null) {
    return;
  }
  (document.getElementById("shorts-active-box") || document.querySelector("shorts-active-box")).style.background = s.bg;
  let sIfr = document.getElementById ( "shorts-iframe" );
  if (sIfr != null) {
    sIfr.src = "https://www.youtube-nocookie.com/embed/" + s.vid + "?autoplay=1&enablejsapi=1&controls=0&loop=1";
  }
  (document.getElementById("shorts-title-text") || document.querySelector("shorts-title-text")).textContent = s.title;
  (document.getElementById("shorts-author-name") || document.querySelector("shorts-author-name")).textContent = s.author;
  (document.getElementById("shorts-audio-text") || document.querySelector("shorts-audio-text")).textContent = s.audio;
  (document.getElementById("shorts-like-count") || document.querySelector("shorts-like-count")).textContent = s.views;
}

function renderChaptersForVideo(vidId, title) {
  let cWrapper = document.getElementById ( "chapters-container" );
  if (cWrapper != null) {
    cWrapper.innerHTML = "";
    let chs = [ { sec : 0 , label : "00:00 • Intro & Overview" } , { sec : 180 , label : "03:00 • Architecture Deep Dive" } , { sec : 420 , label : "07:00 • Implementation & Code" } , { sec : 780 , label : "13:00 • Live Production Demo" } , { sec : 1020 , label : "17:00 • Summary & Benchmarks" } ];
    for (const ch of chs) {
      let btn = document.createElement ( "button" );
      btn.className = "chapter-btn";
      btn.textContent = ch.label;
      btn.onclick = function ( ) { seekToChapter ( ch.sec , ch.label ) };
      cWrapper.appendChild ( btn );
    }
  }
}

async function fetchLiveCommentsForVideo(vidId, title, channel) {
  let cContainer = document.getElementById ( "comment-list-container" );
  if (cContainer == null) {
    return;
  }
  (document.getElementById("comment-list-container") || document.querySelector("comment-list-container")).innerHTML = "<div style='padding:20px; color:#aaa; font-size:14px;'>⏳ Fetching live YouTube comments over internet...</div>";
  let cUrl = "https://invidious.flokinet.to/api/v1/comments/" + vidId;
  try {
    let resp = await fetch ( cUrl );
    let data = await resp.json ( );
    let comments = data.comments || [ ];
    if (comments.length > 0) {
      cContainer.innerHTML = "";
      totalComments = comments.length;
      (document.getElementById("comments-count-header") || document.querySelector("comments-count-header")).textContent = totalComments + " Comments (Live YouTube)";
      for (const c of comments) {
        let card = document.createElement ( "div" );
        card.className = c.isPinned ? "comment-card pinned" : "comment-card";
        let authorName = c.author || "Viewer";
        let authorAvatar = ( c.authorThumbnails && c.authorThumbnails.length > 0 ) ? c.authorThumbnails [ 0 ] .url : "";
        let authorInit = authorName.replace ( / ^ @ / , "" ) .charAt ( 0 ) .toUpperCase ( ) || "Y";
        let pinnedHeader = c.isPinned ? "<span class='pinned-tag'>📌 Pinned by " + channel + "</span>" : "";
        let publishedTime = c.publishedText || "Recently";
        let commentText = c.content || "";
        let likeCount = c.likeCount ? ( c.likeCount > 1000 ? ( c.likeCount / 1000 ) .toFixed ( 1 ) + "K" : c.likeCount ) : "0";
        let avatarHtml = "";
        if (authorAvatar != "") {
          avatarHtml = "<div class='user-avatar-sm'><img src='" + authorAvatar + "' class='comment-avatar-img' onerror=\"this.style.display='none'; this.nextElementSibling.style.display='flex';\" /><span style='display:none;'>" + authorInit + "</span></div>";
        }
        else {
          avatarHtml = "<div class='user-avatar-sm' style='background: #3b82f6;'>" + authorInit + "</div>";
        }
        card.innerHTML = avatarHtml + "<div class='comment-body'>" + pinnedHeader + "<div class='comment-author-row'><span class='comment-author'>" + authorName + "</span><span class='comment-time'>" + publishedTime + "</span></div><p class='comment-msg'>" + commentText + "</p><div class='comment-reactions'><button class='btn-react'>👍 " + likeCount + "</button><button class='btn-react'>👎</button><button class='btn-react'>Reply</button></div></div>";
        cContainer.appendChild ( card );
      }
      showToast("💬 Loaded " + comments.length + " live YouTube comments!");
    }
    else {
      (document.getElementById("comment-list-container") || document.querySelector("comment-list-container")).innerHTML = "<div style='padding:20px; color:#aaa; font-size:14px;'>Be the first to comment on this live stream!</div>";
      (document.getElementById("comments-count-header") || document.querySelector("comments-count-header")).textContent = "0 Comments";
    }
  } catch (err) {
    (document.getElementById("comment-list-container") || document.querySelector("comment-list-container")).innerHTML = "<div style='padding:20px; color:#aaa; font-size:14px;'>Live comments temporarily unavailable. Post a new comment below!</div>";
  }
}

function openWatchPage(vidId, title, channel, views, desc, avatar) {
  currentVideoId = vidId;
  let targetTitle = title || "YouTube Stream Video";
  let targetChannel = channel || "YouTube Creator";
  let targetViews = views || "100K views";
  let targetDesc = desc || ( "Watch " + targetTitle + " streaming in high definition on YouTube." );
  let targetAvatar = avatar || "Y";
  (document.getElementById("watch-video-title") || document.querySelector("watch-video-title")).textContent = targetTitle;
  (document.getElementById("watch-channel-name") || document.querySelector("watch-channel-name")).textContent = targetChannel;
  (document.getElementById("watch-desc-text") || document.querySelector("watch-desc-text")).textContent = targetDesc;
  (document.getElementById("desc-stats") || document.querySelector("desc-stats")).textContent = targetViews + " • Premiered recently";
  let wAvatarEl = document.getElementById ( "watch-channel-avatar" );
  if (wAvatarEl != null) {
    if (targetAvatar.startsWith ( "http" ) || targetAvatar.startsWith ( "//" )) {
      wAvatarEl.innerHTML = "<img src='" + targetAvatar + "' class='channel-avatar-img' onerror=\"this.style.display='none'; this.nextElementSibling.style.display='flex';\" /><span style='display:none;'>" + targetChannel.charAt ( 0 ) + "</span>";
    }
    else {
      wAvatarEl.innerHTML = "<span>" + targetAvatar + "</span>";
    }
  }
  (document.getElementById("share-link-input") || document.querySelector("share-link-input")).value = "https://youtu.be/" + vidId;
  let pIframe = document.getElementById ( "player-iframe" );
  if (pIframe != null) {
    pIframe.src = "https://www.youtube-nocookie.com/embed/" + vidId + "?autoplay=1&enablejsapi=1";
  }
  let ambientEl = document.getElementById ( "player-ambient-light" );
  if (ambientEl != null) {
    ambientEl.style.background = "radial-gradient(circle, rgba(59, 130, 246, 0.35) 0%, rgba(15, 15, 15, 0) 75%)";
  }
  isPlaying = true;
  (document.getElementById("btn-player-play") || document.querySelector("btn-player-play")).textContent = "⏸";
  playSeconds = 0;
  hideAllViews();
  (document.getElementById("view-watch") || document.querySelector("view-watch")).style.display = 'block';
  window.scrollTo ( 0 , 0 );
  renderRecommendations ( vidId );
  renderChaptersForVideo ( vidId , targetTitle );
  fetchLiveCommentsForVideo(vidId , targetTitle , targetChannel);
  showToast("▶ Playing: " + targetTitle);
}

function playVideoItem(item) {
  if (item != null) {
    openWatchPage(item.id , item.title , item.channel , item.views , item.desc , item.avatar);
  }
}

function renderRecommendations(activeVidId) {
  let recList = document.getElementById ( "rec-list-container" );
  if (recList != null) {
    recList.innerHTML = "";
    let others = homeCatalog.filter ( function ( item ) { return item.id !== activeVidId } );
    for (const item of others) {
      let card = document.createElement ( "div" );
      card.className = "rec-card";
      card.innerHTML = "<div class='rec-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='rec-details'><h4 class='rec-title'>" + item.title + "</h4><p class='rec-channel'>" + item.channel + " ✓</p><p class='rec-stats'>" + item.views + "</p></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      recList.appendChild ( card );
    }
  }
}

function renderSubsGrid() {
  let sRow = document.getElementById ( "subs-channels-row" );
  if (sRow != null) {
    sRow.innerHTML = "";
    let subChannels = [ { name : "Spandan Prayas" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } , { name : "MrBeast" , avatar : "https://yt3.ggpht.com/fxGKYucJAVme-YzgnGQru4xphDy00h7-hDxBJqJw-IdbvGiaEgB60eWfJY80cmJUqTv0umGl=s512-c-k-c0x00ffffff-no-rj" } , { name : "JavaScript Mastery" , avatar : "https://yt3.ggpht.com/wg1TITEoPfxvBGfzu-mm1mVmEBoTU4m9Lk0dtoqY4AXjS5ndQm92RrGQMgL2xKTcq0JyNxW9=s512-c-k-c0x00ffffff-no-rj" } , { name : "Snax Gaming" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" } ];
    for (const sc of subChannels) {
      let bubble = document.createElement ( "div" );
      bubble.className = "channel-story-bubble";
      bubble.innerHTML = "<div class='channel-story-avatar'><img src='" + sc.avatar + "' class='channel-avatar-img' /></div><span class='channel-story-name'>" + sc.name + "</span>";
      bubble.onclick = function ( ) { searchLiveYouTube ( sc.name ) };
      sRow.appendChild ( bubble );
    }
  }
  let sGrid = document.getElementById ( "subs-grid-container" );
  if (sGrid != null) {
    sGrid.innerHTML = "";
    for (const item of subsCatalog) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      let avHtml = ( item.avatar && item.avatar.startsWith ( "http" ) ) ? "<div class='channel-avatar'><img src='" + item.avatar + "' class='channel-avatar-img' /></div>" : "<div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div>";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'>" + avHtml + "<div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      sGrid.appendChild ( card );
    }
  }
}

function renderLibraryView() {
  let hContainer = document.getElementById ( "history-list-container" );
  if (hContainer != null) {
    hContainer.innerHTML = "";
    for (const item of historyCatalog) {
      let card = document.createElement ( "div" );
      card.className = "history-card";
      card.innerHTML = "<div class='history-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='history-details'><h3 class='history-title'>" + item.title + "</h3><p class='history-sub'>" + item.channel + " • " + item.views + "</p><p class='history-time'>" + item.time + "</p></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      hContainer.appendChild ( card );
    }
  }
  let wlContainer = document.getElementById ( "watch-later-container" );
  if (wlContainer != null) {
    wlContainer.innerHTML = "";
    for (const item of watchLaterCatalog) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      let avHtml = ( item.avatar && item.avatar.startsWith ( "http" ) ) ? "<div class='channel-avatar'><img src='" + item.avatar + "' class='channel-avatar-img' /></div>" : "<div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div>";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'>" + avHtml + "<div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      wlContainer.appendChild ( card );
    }
  }
  let lkContainer = document.getElementById ( "liked-videos-container" );
  if (lkContainer != null) {
    lkContainer.innerHTML = "";
    for (const item of likedCatalog) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      let avHtml = ( item.avatar && item.avatar.startsWith ( "http" ) ) ? "<div class='channel-avatar'><img src='" + item.avatar + "' class='channel-avatar-img' /></div>" : "<div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div>";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'>" + avHtml + "<div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      lkContainer.appendChild ( card );
    }
  }
}

function renderTrendingList() {
  let tContainer = document.getElementById ( "trending-list-container" );
  if (tContainer != null) {
    tContainer.innerHTML = "";
    for (const item of trendingCatalog) {
      let card = document.createElement ( "div" );
      card.className = "trending-card";
      card.innerHTML = "<div class='trending-rank-badge'>#" + item.rank + "</div><div class='history-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='history-details'><h3 class='history-title'>" + item.title + "</h3><p class='history-sub'>" + item.channel + " ✓ • " + item.views + "</p><p class='desc-content' style='color:#aaa; font-size:12px;'>" + item.desc + "</p></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      tContainer.appendChild ( card );
    }
  }
}

function renderGrid(items) {
  let grid = document.getElementById ( "video-grid-container" );
  if (grid != null) {
    grid.innerHTML = "";
    for (const item of items) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      card.id = "card-" + item.id;
      let avatarHtml = "";
      if (item.avatar != null && ( item.avatar.startsWith ( "http" ) || item.avatar.startsWith ( "//" ) )) {
        avatarHtml = "<div class='channel-avatar'><img src='" + item.avatar + "' class='channel-avatar-img' onerror=\"this.style.display='none'; this.nextElementSibling.style.display='flex';\" /><span style='display:none;'>" + item.channel.charAt ( 0 ) + "</span></div>";
      }
      else {
        avatarHtml = "<div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div>";
      }
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' onerror=\"this.src='https://i.ytimg.com/vi/" + item.id + "/mqdefault.jpg'\" /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'>" + avatarHtml + "<div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      grid.appendChild ( card );
    }
  }
}

async function loadMoreContent() {
  currentFeedPage = currentFeedPage + 1;
  let grid = document.getElementById ( "video-grid-container" );
  if (grid == null) {
    return;
  }
  showToast("⏳ Loading page " + currentFeedPage + " from live YouTube...");
  if (currentSearchQuery != "") {
    let targetUrl = "https://invidious.flokinet.to/api/v1/search?q=" + encodeURIComponent ( currentSearchQuery ) + "&page=" + currentFeedPage + "&type=video";
    try {
      let resp = await fetch ( targetUrl );
      let apiList = await resp.json ( );
      if (apiList != null && apiList.length > 0) {
        for (const item of apiList) {
          let vId = item.videoId;
          let vTitle = item.title || "YouTube Video";
          let vAuthor = item.author || "YouTube Creator";
          let vAvatar = ( item.authorThumbnails && item.authorThumbnails.length > 0 ) ? item.authorThumbnails [ item.authorThumbnails.length - 1 ] .url : "";
          let vViews = ( item.viewCount ? ( item.viewCount > 1000000 ? ( item.viewCount / 1000000 ) .toFixed ( 1 ) + "M views" : ( item.viewCount / 1000 ) .toFixed ( 0 ) + "K views" ) : "100K views" ) + " • " + ( item.publishedText || "Recently" );
          let vDur = item.lengthSeconds ? ( Math.floor ( item.lengthSeconds / 60 ) + ":" + ( ( item.lengthSeconds % 60 < 10 ? "0" : "" ) + ( item.lengthSeconds % 60 ) ) ) : "10:00";
          let vDesc = item.description || ( "Watch " + vTitle + " live on YouTube." );
          let cardObj = { id : vId , title : vTitle , channel : vAuthor , views : vViews , duration : vDur , desc : vDesc , avatar : vAvatar , category : "All" };
          let card = document.createElement ( "div" );
          card.className = "video-card";
          card.id = "card-" + vId;
          let avHtml = ( vAvatar != "" ) ? "<div class='channel-avatar'><img src='" + vAvatar + "' class='channel-avatar-img' onerror=\"this.style.display='none'; this.nextElementSibling.style.display='flex';\" /><span style='display:none;'>" + vAuthor.charAt ( 0 ) + "</span></div>" : "<div class='channel-avatar'>" + vAuthor.charAt ( 0 ) + "</div>";
          card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + vId + "/hqdefault.jpg' class='thumb-real-img' onerror=\"this.src='https://i.ytimg.com/vi/" + vId + "/mqdefault.jpg'\" /><span class='duration-badge'>" + vDur + "</span></div><div class='video-meta'>" + avHtml + "<div class='video-details'><h3 class='video-title'>" + vTitle + "</h3><p class='video-channel'>" + vAuthor + " ✓</p><p class='video-stats'>" + vViews + "</p></div></div>";
          card.onclick = function ( ) { playVideoItem ( cardObj ) };
          grid.appendChild ( card );
        }
        showToast("✨ Loaded " + apiList.length + " more live YouTube videos for '" + currentSearchQuery + "'!");
      }
    } catch (err) {
      showToast("No more live videos found for this search");
    }
  }
  else {
    let pool = homeCatalog;
    if (currentCategoryFilter != "All") {
      let filtered = homeCatalog.filter ( function ( item ) { return item.category == currentCategoryFilter } );
      if (filtered.length > 0) {
        pool = filtered;
      }
    }
    for (const item of pool) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      let avHtml = ( item.avatar && item.avatar.startsWith ( "http" ) ) ? "<div class='channel-avatar'><img src='" + item.avatar + "' class='channel-avatar-img' /></div>" : "<div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div>";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'>" + avHtml + "<div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { playVideoItem ( item ) };
      grid.appendChild ( card );
    }
    showToast("✨ Loaded " + pool.length + " more videos into home feed!");
  }
}

function filterByCategory(categoryName) {
  currentCategoryFilter = categoryName;
  let allChips = document.querySelectorAll ( ".chip" );
  allChips.forEach ( function ( chip ) { chip.classList.remove ( "active" ) } );
  if (categoryName == "All") {
    (document.getElementById("chip-all") || document.querySelector("chip-all")).classList.add('active');
    renderGrid(homeCatalog);
    (document.getElementById("live-status-title") || document.querySelector("live-status-title")).textContent = "🔴 All YouTube Feed";
    (document.getElementById("live-status-desc") || document.querySelector("live-status-desc")).textContent = "Displaying " + homeCatalog.length + " live videos across all genres • Infinite Scroll active";
  }
  else {
    let filtered = homeCatalog.filter ( function ( item ) { return item.category == categoryName } );
    if (filtered.length == 0) {
      filtered = homeCatalog;
    }
    renderGrid(filtered);
    (document.getElementById("live-status-title") || document.querySelector("live-status-title")).textContent = "🔴 Category: " + categoryName;
    (document.getElementById("live-status-desc") || document.querySelector("live-status-desc")).textContent = "Displaying " + filtered.length + " curated live YouTube videos for " + categoryName;
  }
  showToast("Showing category: " + categoryName);
}

function matchVideoQuery(v, qLow) {
  let fullText = ( v.title + " " + v.channel + " " + v.desc + " " + v.category ) .toLowerCase ( );
  if (fullText.indexOf ( qLow ) != - 1) {
    return true;
  }
  if (qLow.indexOf ( "gta" ) != - 1 && fullText.indexOf ( "grand theft auto" ) != - 1) {
    return true;
  }
  if (qLow.indexOf ( "squid" ) != - 1 && fullText.indexOf ( "squid" ) != - 1) {
    return true;
  }
  if (qLow.indexOf ( "mrbeast" ) != - 1 && fullText.indexOf ( "mrbeast" ) != - 1) {
    return true;
  }
  return false;
}

async function fetchLiveYouTubeFromWeb(query) {
  (document.getElementById("live-status-title") || document.querySelector("live-status-title")).textContent = "⚡ Fetching live YouTube stream results for: " + query + "...";
  (document.getElementById("live-status-desc") || document.querySelector("live-status-desc")).textContent = "Connecting to live YouTube API stream...";
  let targetUrl = "https://invidious.flokinet.to/api/v1/search?q=" + encodeURIComponent ( query ) + "&page=1&type=video";
  try {
    let response = await fetch ( targetUrl );
    let apiList = await response.json ( );
    if (apiList != null && apiList.length > 0) {
      let liveCards = [ ];
      for (const item of apiList) {
        let vId = item.videoId;
        let vTitle = item.title || "YouTube Video";
        let vAuthor = item.author || "YouTube Creator";
        let vAvatar = ( item.authorThumbnails && item.authorThumbnails.length > 0 ) ? item.authorThumbnails [ item.authorThumbnails.length - 1 ] .url : "";
        let vViews = ( item.viewCount ? ( item.viewCount > 1000000 ? ( item.viewCount / 1000000 ) .toFixed ( 1 ) + "M views" : ( item.viewCount / 1000 ) .toFixed ( 0 ) + "K views" ) : "100K views" ) + " • " + ( item.publishedText || "Recently" );
        let vDur = item.lengthSeconds ? ( Math.floor ( item.lengthSeconds / 60 ) + ":" + ( ( item.lengthSeconds % 60 < 10 ? "0" : "" ) + ( item.lengthSeconds % 60 ) ) ) : "10:00";
        let vDesc = item.description || ( "Watch " + vTitle + " live on YouTube." );
        liveCards.push ( { id : vId , title : vTitle , channel : vAuthor , views : vViews , duration : vDur , desc : vDesc , avatar : vAvatar , category : "All" } );
      }
      renderGrid(liveCards);
      (document.getElementById("live-status-title") || document.querySelector("live-status-title")).textContent = "🔴 Live YouTube Search Results for: " + query;
      (document.getElementById("live-status-desc") || document.querySelector("live-status-desc")).textContent = "Fetched " + liveCards.length + " real YouTube videos live with official channel logos";
      showToast("🎬 Fetched " + liveCards.length + " live YouTube videos for '" + query + "'");
    }
  } catch (err) {
    showToast("Displaying instant matches for '" + query + "'");
  }
}

function searchLiveYouTube(query) {
  let q = ( query || "" ) .trim ( );
  if (q == "") {
    currentSearchQuery = "";
    currentFeedPage = 1;
    renderGrid(homeCatalog);
    return;
  }
  currentSearchQuery = q;
  currentFeedPage = 1;
  let directVidId = "";
  if (q.length == 11 && q.indexOf ( " " ) == - 1) {
    directVidId = q;
  }
  else {
    if (q.includes ( "youtube.com/watch?v=" )) {
      directVidId = q.split ( "v=" ) [ 1 ] .split ( "&" ) [ 0 ];
    }
    else {
      if (q.includes ( "youtu.be/" )) {
        directVidId = q.split ( "youtu.be/" ) [ 1 ] .split ( "?" ) [ 0 ];
      }
    }
  }
  if (directVidId != "") {
    openWatchPage(directVidId , ( "Live Video Stream (" + directVidId + ")" ) , "YouTube Direct Stream" , "1 view • Just loaded" , "Direct stream loaded from YouTube URL." , "Y");
    showToast("🎬 Directly playing YouTube stream: " + directVidId);
    return;
  }
  let qLow = q.toLowerCase ( );
  let matches = homeCatalog.filter ( function ( v ) { return matchVideoQuery ( v , qLow ) } );
  hideAllViews();
  (document.getElementById("view-feed") || document.querySelector("view-feed")).style.display = 'block';
  if (matches.length > 0) {
    renderGrid(matches);
  }
  else {
    renderGrid(homeCatalog);
  }
  fetchLiveYouTubeFromWeb(q);
}

renderGrid(homeCatalog);

let searchInpEl = document.getElementById ( "search-input" );

if (searchInpEl != null) {
  searchInpEl.onkeyup = function ( e ) { if ( e.key === "Enter" ) { searchLiveYouTube ( searchInpEl.value ) } };
  searchInpEl.oninput = function ( ) { if ( searchInpEl.value.length >= 3 ) { searchLiveYouTube ( searchInpEl.value ) } };
}

let scrubberEl = document.getElementById ( "player-scrubber" );

if (scrubberEl != null) {
  scrubberEl.onclick = function ( e ) { seekScrubber ( e ) };
}

let shortsContEl = document.getElementById ( "view-shorts" );

if (shortsContEl != null) {
  shortsContEl.onwheel = function ( e ) { if ( e.deltaY > 30 ) { renderActiveShort ( currentShortIndex + 1 ) } if ( e.deltaY < - 30 ) { renderActiveShort ( currentShortIndex - 1 ) } };
}

(function() {
  const targetEl = (document.getElementById("nav-home") || document.querySelector("nav-home"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openHomeFeed();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-logo") || document.querySelector("btn-logo"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openHomeFeed();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-shorts") || document.querySelector("nav-shorts"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openShortsReel();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-subs") || document.querySelector("nav-subs"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openSubsFeed();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-library") || document.querySelector("nav-library"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openLibrary();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-history") || document.querySelector("nav-history"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openLibrary();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-your-videos") || document.querySelector("nav-your-videos"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openSubsFeed();
      showToast("🎬 Your uploaded videos (Spandan Prayas Patra)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-watch-later") || document.querySelector("nav-watch-later"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openLibrary();
      showToast("⏱️ Watch Later playlist (14 videos)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-liked") || document.querySelector("nav-liked"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openLibrary();
      showToast("👍 Liked videos playlist (86 videos)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("sub-1") || document.querySelector("sub-1"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Java Backend");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("sub-2") || document.querySelector("sub-2"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Enlang Stack");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("sub-3") || document.querySelector("sub-3"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Next.js");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("sub-4") || document.querySelector("sub-4"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Coding & AI");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("sub-5") || document.querySelector("sub-5"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      searchLiveYouTube("MrBeast");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-trending") || document.querySelector("nav-trending"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openTrending();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-music") || document.querySelector("nav-music"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Lo-Fi Music");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-gaming") || document.querySelector("nav-gaming"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Gaming");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-news") || document.querySelector("nav-news"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      searchLiveYouTube("World Tech News");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-sports") || document.querySelector("nav-sports"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      searchLiveYouTube("Cricket Highlights");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-load-more") || document.querySelector("btn-load-more"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      loadMoreContent();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-sidebar-toggle") || document.querySelector("btn-sidebar-toggle"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("yt-sidebar") || document.querySelector("yt-sidebar")).classList.toggle('collapsed');
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-create") || document.querySelector("btn-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-create") || document.querySelector("modal-create")).style.display = 'block';
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-create") || document.querySelector("btn-close-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-create") || document.querySelector("modal-create")).style.display = 'none';
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-upload") || document.querySelector("btn-cancel-upload"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-create") || document.querySelector("modal-create")).style.display = 'none';
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-publish-video") || document.querySelector("btn-publish-video"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let uTitleInp = document.getElementById ( "upload-title-input" );
      let uTitle = ( uTitleInp != null && uTitleInp.value ) ? uTitleInp.value : "New Enlang Video by Spandan Prayas Patra";
      let newV = { id : "gJrjgg1KVL4" , title : uTitle , channel : "Spandan Prayas Patra" , subs : "128K subscribers" , views : "1 view • Just now" , desc : "Newly uploaded creator video in Enlang." , category : "All" , duration : "12:30" , avatar : "https://yt3.ggpht.com/SF6gtLA16ykecLQ-bEgYReAlazH0giurl5zfQyBMAClMofvSEE7IiKSdbFNDf40EcNYzKwPjfA=s512-c-k-c0x00ffffff-no-rj" };
      homeCatalog.unshift ( newV );
      openHomeFeed();
      (document.getElementById("modal-create") || document.querySelector("modal-create")).style.display = 'none';
      showToast("🚀 Video Published Live: " + uTitle);
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-notif") || document.querySelector("btn-notif"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let dNotif = document.getElementById ( "dropdown-notif" );
      if (dNotif != null) {
        if (dNotif.style.display == "flex") {
          (document.getElementById("dropdown-notif") || document.querySelector("dropdown-notif")).style.display = 'none';
        }
        else {
          dNotif.style.display = "flex";
        }
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-voice") || document.querySelector("btn-voice"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🎙️ Listening... Say 'Snax Gaming', 'Spring Boot', or 'MrBeast'");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("user-profile") || document.querySelector("user-profile"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let dProf = document.getElementById ( "dropdown-profile" );
      if (dProf != null) {
        if (dProf.style.display == "flex") {
          (document.getElementById("dropdown-profile") || document.querySelector("dropdown-profile")).style.display = 'none';
        }
        else {
          dProf.style.display = "flex";
        }
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-channel") || document.querySelector("btn-profile-channel"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openSubsFeed();
      showToast("👤 Opening Your Channel (@SpandanPrayas)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-studio") || document.querySelector("btn-profile-studio"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🎬 YouTube Studio Creator Dashboard");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-switch") || document.querySelector("btn-profile-switch"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🔄 Account Switcher Dialog");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-signout") || document.querySelector("btn-profile-signout"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🚪 Signed out of YouTube");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-location") || document.querySelector("btn-profile-location"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🌐 Location: India (IN)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-share") || document.querySelector("btn-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-share") || document.querySelector("modal-share")).style.display = 'block';
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-share") || document.querySelector("btn-close-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-share") || document.querySelector("modal-share")).style.display = 'none';
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-copy-share") || document.querySelector("btn-copy-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("modal-share") || document.querySelector("modal-share")).style.display = 'none';
      showToast("🔗 Link copied to clipboard!");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-search") || document.querySelector("btn-search"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sInpEl = document.getElementById ( "search-input" );
      if (sInpEl != null) {
        searchLiveYouTube(sInpEl.value);
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-all") || document.querySelector("chip-all"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("All");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-coding") || document.querySelector("chip-coding"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Coding & AI");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-java") || document.querySelector("chip-java"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Java Backend");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-spring") || document.querySelector("chip-spring"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Java Backend");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-enlang") || document.querySelector("chip-enlang"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Enlang Stack");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-sysdesign") || document.querySelector("chip-sysdesign"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("System Design");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-next") || document.querySelector("chip-next"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Next.js");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-edge") || document.querySelector("chip-edge"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Next.js");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-music") || document.querySelector("chip-music"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Lo-Fi Music");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-gaming") || document.querySelector("chip-gaming"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Gaming");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-science") || document.querySelector("chip-science"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("Science & Tech");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("chip-podcasts") || document.querySelector("chip-podcasts"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      filterByCategory("System Design");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("short-shelf-1") || document.querySelector("short-shelf-1"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openShortsReel();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("short-shelf-2") || document.querySelector("short-shelf-2"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openShortsReel();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("short-shelf-3") || document.querySelector("short-shelf-3"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openShortsReel();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("short-shelf-4") || document.querySelector("short-shelf-4"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      openShortsReel();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-play") || document.querySelector("btn-player-play"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      if (isPlaying == true) {
        isPlaying = false;
        (document.getElementById("btn-player-play") || document.querySelector("btn-player-play")).textContent = "▶";
        sendPlayerCmd("pauseVideo" , [ ]);
        showToast("⏸ Stream paused");
      }
      else {
        isPlaying = true;
        (document.getElementById("btn-player-play") || document.querySelector("btn-player-play")).textContent = "⏸";
        sendPlayerCmd("playVideo" , [ ]);
        showToast("▶ Stream playing");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-next") || document.querySelector("btn-player-next"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let nextItem = homeCatalog [ 1 ];
      openWatchPage(nextItem.id , nextItem.title , nextItem.channel , nextItem.views , nextItem.desc , nextItem.avatar);
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-mute") || document.querySelector("btn-player-mute"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      if (isMuted == true) {
        isMuted = false;
        (document.getElementById("btn-player-mute") || document.querySelector("btn-player-mute")).textContent = "🔊";
        sendPlayerCmd("unMute" , [ ]);
        showToast("Audio unmuted (100%)");
      }
      else {
        isMuted = true;
        (document.getElementById("btn-player-mute") || document.querySelector("btn-player-mute")).textContent = "🔇";
        sendPlayerCmd("mute" , [ ]);
        showToast("Audio muted");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-speed") || document.querySelector("btn-player-speed"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sBtn = document.getElementById ( "btn-player-speed" );
      let rate = 1;
      if (playbackSpeed == "1x") {
        playbackSpeed = "1.5x";
        rate = 1.5;
      }
      else {
        if (playbackSpeed == "1.5x") {
          playbackSpeed = "2x";
          rate = 2;
        }
        else {
          if (playbackSpeed == "2x") {
            playbackSpeed = "0.5x";
            rate = 0.5;
          }
          else {
            playbackSpeed = "1x";
            rate = 1;
          }
        }
      }
      if (sBtn != null) {
        sBtn.innerText = playbackSpeed;
      }
      sendPlayerCmd("setPlaybackRate" , [ rate ]);
      showToast("⚡ Playback Speed: " + playbackSpeed);
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-quality") || document.querySelector("btn-player-quality"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let qList = [ "1080p HD" , "4K (2160p)" , "1440p QHD" , "720p HD" , "480p" , "Auto" ];
      qualityIndex = ( qualityIndex + 1 ) % qList.length;
      let curQ = qList [ qualityIndex ];
      (document.getElementById("btn-player-quality") || document.querySelector("btn-player-quality")).textContent = "⚙️ " + curQ;
      sendPlayerCmd("setPlaybackQuality" , [ curQ.toLowerCase ( ) ]);
      showToast("⚙️ Quality set to " + curQ);
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-cc") || document.querySelector("btn-player-cc"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("📝 Closed Captions: English (Auto-generated)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-fullscreen") || document.querySelector("btn-player-fullscreen"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let pCont = document.querySelector ( ".video-player-container" );
      if (document.fullscreenElement == null) {
        if (pCont != null) {
          pCont.requestFullscreen ( );
          showToast("Entered Fullscreen Player");
        }
      }
      else {
        document.exitFullscreen ( );
        showToast("Exited Fullscreen Player");
      }
    });
  }
})();

setInterval(function() {
  if (isPlaying == true) {
    if (playSeconds < totalVideoSeconds) {
      playSeconds = playSeconds + 1;
      let curStr = formatDuration ( playSeconds );
      let totStr = formatDuration ( totalVideoSeconds );
      (document.getElementById("player-time-display") || document.querySelector("player-time-display")).textContent = curStr + " / " + totStr;
      let pct = ( playSeconds / totalVideoSeconds ) * 100;
      let fillBar = document.getElementById ( "scrubber-progress" );
      if (fillBar != null) {
        fillBar.style.width = pct + "%";
      }
    }
  }
}, 1000);

(function() {
  const targetEl = (document.getElementById("ch-1") || document.querySelector("ch-1"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      playSeconds = 0;
      sendPlayerCmd("seekTo" , [ 0 , true ]);
      showToast("📍 Chapter: 00:00 • Introduction");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ch-2") || document.querySelector("ch-2"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      playSeconds = 255;
      sendPlayerCmd("seekTo" , [ 255 , true ]);
      showToast("📍 Chapter: 04:15 • Setup");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ch-3") || document.querySelector("ch-3"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      playSeconds = 570;
      sendPlayerCmd("seekTo" , [ 570 , true ]);
      showToast("📍 Chapter: 09:30 • Core Logic");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ch-4") || document.querySelector("ch-4"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      playSeconds = 840;
      sendPlayerCmd("seekTo" , [ 840 , true ]);
      showToast("📍 Chapter: 14:00 • Benchmarks");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-join") || document.querySelector("btn-join"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("⭐ Welcome! Channel Membership perks unlocked.");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-subscribe") || document.querySelector("btn-subscribe"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sBtn = document.getElementById ( "btn-subscribe" );
      if (isSubscribed == true) {
        isSubscribed = false;
        (document.getElementById("btn-subscribe") || document.querySelector("btn-subscribe")).textContent = "Subscribe";
        if (sBtn != null) {
          sBtn.style.background = "#ffffff";
          sBtn.style.color = "#0f0f0f";
        }
        showToast("Unsubscribed from Creator");
      }
      else {
        isSubscribed = true;
        (document.getElementById("btn-subscribe") || document.querySelector("btn-subscribe")).textContent = "Subscribed ✓";
        if (sBtn != null) {
          sBtn.style.background = "#272727";
          sBtn.style.color = "#aaaaaa";
        }
        showToast("🔔 Subscribed to Creator!");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-like") || document.querySelector("btn-like"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let lBtn = document.getElementById ( "btn-like" );
      if (isLiked == true) {
        isLiked = false;
        (document.getElementById("btn-like") || document.querySelector("btn-like")).textContent = "👍 142K";
        if (lBtn != null) {
          lBtn.style.color = "#f1f1f1";
        }
      }
      else {
        isLiked = true;
        (document.getElementById("btn-like") || document.querySelector("btn-like")).textContent = "👍 142.1K";
        if (lBtn != null) {
          lBtn.style.color = "#3ea6ff";
        }
        showToast("Added to Liked Videos");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-dislike") || document.querySelector("btn-dislike"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("Disliked video");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-download") || document.querySelector("btn-download"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("⬇ Downloading 1080p stream for offline playback...");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-clip") || document.querySelector("btn-clip"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("✂ Create Clip dialog opened");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-more") || document.querySelector("btn-more"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("⋯ Options: Save to Playlist, Report, Transcript");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-next") || document.querySelector("btn-shorts-next"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      renderActiveShort(( currentShortIndex + 1 ));
      showToast("Next Short loaded");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-prev") || document.querySelector("btn-shorts-prev"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      renderActiveShort(( currentShortIndex - 1 ));
      showToast("Previous Short loaded");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-like") || document.querySelector("btn-shorts-like"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      if (isShortsLiked == true) {
        isShortsLiked = false;
        (document.getElementById("shorts-like-count") || document.querySelector("shorts-like-count")).textContent = "245K";
      }
      else {
        isShortsLiked = true;
        (document.getElementById("shorts-like-count") || document.querySelector("shorts-like-count")).textContent = "245.1K";
        showToast("Liked Short!");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-dislike") || document.querySelector("btn-shorts-dislike"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("Disliked Short");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-comment") || document.querySelector("btn-shorts-comment"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("💬 Opening Shorts Comments (4.2K)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-share") || document.querySelector("btn-shorts-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("🔗 Shorts link copied!");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-sub") || document.querySelector("btn-shorts-sub"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sBtn = document.getElementById ( "btn-shorts-sub" );
      if (sBtn != null) {
        if (sBtn.innerText == "Subscribe") {
          (document.getElementById("btn-shorts-sub") || document.querySelector("btn-shorts-sub")).textContent = "Subscribed ✓";
          sBtn.style.background = "#272727";
          sBtn.style.color = "#aaaaaa";
          showToast("Subscribed to Creator!");
        }
        else {
          (document.getElementById("btn-shorts-sub") || document.querySelector("btn-shorts-sub")).textContent = "Subscribe";
          sBtn.style.background = "#ffffff";
          sBtn.style.color = "#0f0f0f";
        }
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-mute") || document.querySelector("btn-shorts-mute"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      sendShortsCmd("mute" , [ ]);
      showToast("Shorts audio toggled");
    });
  }
})();

function postComment() {
  let cInp = document.getElementById ( "new-comment-input" );
  if (cInp == null || cInp.value.trim ( ) == "") {
    showToast("Please type a comment before posting");
    return;
  }
  let cText = cInp.value.trim ( );
  let cContainer = document.getElementById ( "comment-list-container" );
  if (cContainer != null) {
    let cCard = document.createElement ( "div" );
    cCard.className = "comment-card";
    cCard.innerHTML = "<div class='user-avatar-sm' style='background: #8b5cf6;'>S</div><div class='comment-body'><div class='comment-author-row'><span class='comment-author'>@SpandanPrayas (You)</span><span class='comment-time'>Just now</span></div><p class='comment-msg'>" + cText + "</p><div class='comment-reactions'><button class='btn-react'>👍 1</button><button class='btn-react'>👎</button><button class='btn-react'>Reply</button></div></div>";
    cContainer.prepend ( cCard );
    cInp.value = "";
    totalComments = totalComments + 1;
    (document.getElementById("comments-count-header") || document.querySelector("comments-count-header")).textContent = totalComments + " Comments";
    showToast("💬 Comment posted publicly!");
  }
}

(function() {
  const targetEl = (document.getElementById("btn-submit-comment") || document.querySelector("btn-submit-comment"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      postComment();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-comment") || document.querySelector("btn-cancel-comment"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("new-comment-input") || document.querySelector("new-comment-input")).value = "";
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-clear-history") || document.querySelector("btn-clear-history"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      (document.getElementById("history-list-container") || document.querySelector("history-list-container")).innerHTML = "<p style='color: #888; padding: 20px;'>Watch history is empty.</p>";
      showToast("🗑️ Watch History Cleared");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-theme") || document.querySelector("btn-theme"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let b = document.body;
      if (b.classList.contains ( "light-theme" )) {
        document.body.classList.remove('light-theme');
        (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☾";
        showToast("Switched to YouTube Dark Mode");
      }
      else {
        document.body.classList.add('light-theme');
        (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☼";
        showToast("Switched to YouTube Light Mode");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-profile-appearance") || document.querySelector("btn-profile-appearance"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let b = document.body;
      if (b.classList.contains ( "light-theme" )) {
        document.body.classList.remove('light-theme');
        (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☾";
        showToast("Switched to YouTube Dark Mode");
      }
      else {
        document.body.classList.add('light-theme');
        (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☼";
        showToast("Switched to YouTube Light Mode");
      }
    });
  }
})();
    