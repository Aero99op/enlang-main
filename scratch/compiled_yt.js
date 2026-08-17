let isPlaying = true;

let isMuted = false;

let isSubscribed = false;

let isLiked = false;

let currentVideoId = "gJrjgg1KVL4";

let currentShortIndex = 0;

let totalComments = 328;

let playSeconds = 255;

let totalVideoSeconds = 4282;

let playbackSpeed = "1x";

let isShortsLiked = false;

let currentCategoryFilter = "All";

let feedRenderCount = 0;

console.log("▶ Initializing 1:1 Infinite YouTube Engine in Pure Enlang (.enlgs)...");

const masterCatalog = [ { id : "gJrjgg1KVL4" , title : "Spring Boot 3 Full Course 2025 - Beginner to Pro" , channel : "Spandan Prayas Patra" , subs : "128K subscribers" , views : "1,039,987 views • Premiered on Aug 15, 2026" , desc : "In this deep dive tutorial, Spandan Prayas Patra walks through architecting mission-critical Java 21 backend microservices, configuring HikariCP connection pools for Oracle & MySQL, and applying strict ACID transaction management." , category : "Java Backend" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "1:11:22" , avatar : "S" } , { id : "0e3GPea1Tyg" , title : "$456,000 Squid Game In Real Life!" , channel : "MrBeast" , subs : "310M subscribers" , views : "630,240,119 views • 2 years ago" , desc : "456 people compete for $456,000 in real life recreation of Squid Game! Red Light Green Light, Dalgona, Tug of War, Marbles, Glass Bridge, and the Final Battle." , category : "Science & Tech" , ambient : "rgba(239, 68, 68, 0.35)" , duration : "25:41" , avatar : "M" } , { id : "dQw4w9WgXcQ" , title : "Rick Astley - Never Gonna Give You Up (Official Music Video 4K Remaster)" , channel : "Rick Astley" , subs : "3.4M subscribers" , views : "1,520,840,112 views • 14 years ago" , desc : "The official video for 'Never Gonna Give You Up' by Rick Astley. 4K Remastered with studio high definition audio." , category : "Lo-Fi Music" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "03:33" , avatar : "R" } , { id : "8aGhZQkoFbQ" , title : "Next.js 15 & React 19 Full Stack Masterclass (App Router, Server Actions)" , channel : "JavaScript Mastery" , subs : "1.1M subscribers" , views : "489,420 views • 3 weeks ago" , desc : "Master full-stack Next.js with Server Actions, Edge Middleware, and Cloudflare Deployments with zero 500 errors." , category : "Next.js" , ambient : "rgba(16, 185, 129, 0.35)" , duration : "2:45:10" , avatar : "J" } , { id : "HXV3zeRR3h4" , title : "System Design for High Scale - 100 Million Active Users" , channel : "ByteByteGo" , subs : "1.9M subscribers" , views : "980,120 views • 1 month ago" , desc : "An insider look into database sharding, distributed caches, and load balancers at massive hyperscale." , category : "System Design" , ambient : "rgba(249, 115, 22, 0.35)" , duration : "18:24" , avatar : "B" } , { id : "L_LUpnjgPso" , title : "Grand Theft Auto VI Official Trailer 1 - 4K 60FPS" , channel : "Rockstar Games" , subs : "10.8M subscribers" , views : "215,900,000 views • 8 months ago" , desc : "Grand Theft Auto VI heads to the state of Leonida, home to the neon-soaked streets of Vice City." , category : "Gaming" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "01:31" , avatar : "R" } , { id : "1-2J_aFkYlY" , title : "Apple Vision Pro Review: Tomorrow's Tech Today!" , channel : "Marques Brownlee" , subs : "19.2M subscribers" , views : "18,400,000 views • 6 months ago" , desc : "Apple Vision Pro is here. The eye tracking and spatial computing are incredible, but what about battery life and weight?" , category : "Science & Tech" , ambient : "rgba(99, 102, 241, 0.35)" , duration : "21:05" , avatar : "M" } , { id : "Q7AOvWpIVHU" , title : "Three.js 3D Web Graphics & WebGL Shaders Masterclass" , channel : "Creative Coding 3D" , subs : "340K subscribers" , views : "340,900 views • 5 days ago" , desc : "Shader art tutorial implementing dual plasma rings and procedural celestial textures in WebGL." , category : "Coding & AI" , ambient : "rgba(99, 102, 241, 0.35)" , duration : "21:15" , avatar : "T" } , { id : "pTJJsmejUOQ" , title : "Flutter 3.24 & Dart Full Course for Cross-Platform Mobile Apps" , channel : "FreeCodeCamp" , subs : "9.8M subscribers" , views : "1,420,150 views • 2 months ago" , desc : "Build cross-platform iOS & Android mobile applications with clean architecture and state management." , category : "Enlang Stack" , ambient : "rgba(20, 184, 166, 0.35)" , duration : "4:32:00" , avatar : "F" } , { id : "bhiE28f9q4c" , title : "Glitter Bomb 5.0 vs. Porch Pirates" , channel : "Mark Rober" , subs : "56.4M subscribers" , views : "48,900,000 views • 1 year ago" , desc : "The final and most engineering-heavy iteration of the glitter bomb trap for package thieves." , category : "Science & Tech" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "24:18" , avatar : "M" } , { id : "fJ9rUzIMcZQ" , title : "Queen - Bohemian Rhapsody (Official Video Remastered)" , channel : "Queen Official" , subs : "18.2M subscribers" , views : "1,740,290,000 views • 15 years ago" , desc : "The official music video for Queen's iconic Bohemian Rhapsody, remastered in HD." , category : "Lo-Fi Music" , ambient : "rgba(168, 85, 247, 0.35)" , duration : "05:59" , avatar : "Q" } , { id : "kXYiU_JCYtU" , title : "Linkin Park - Numb (Official Music Video 4K)" , channel : "Linkin Park" , subs : "21.4M subscribers" , views : "2,210,000,000 views • 17 years ago" , desc : "'Numb' by Linkin Park from the album Meteora. 4K Ultra High Definition remastered." , category : "Lo-Fi Music" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "03:07" , avatar : "L" } , { id : "jfKfPfyJRdk" , title : "lofi hip hop radio 📚 - beats to relax/study to" , channel : "Lofi Girl" , subs : "14.1M subscribers" , views : "98,400,000 views • Live 24/7" , desc : "Peaceful lofi hip hop beats to relax, study, code, and sleep to. Welcome to the official Lofi Girl live stream." , category : "Lo-Fi Music" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "LIVE" , avatar : "L" } , { id : "Un5SEJ8MyPc" , title : "Cyberpunk 2077: Phantom Liberty - Official Cinematic Trailer" , channel : "Cyberpunk 2077" , subs : "1.4M subscribers" , views : "14,200,000 views • 1 year ago" , desc : "Return to Night City in this spy-thriller expansion featuring Idris Elba as Solomon Reed." , category : "Gaming" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "04:12" , avatar : "C" } , { id : "MBRqu0YOH14" , title : "The Egg - A Short Story" , channel : "Kurzgesagt – In a Nutshell" , subs : "22.8M subscribers" , views : "34,200,000 views • 4 years ago" , desc : "A philosophical animation of Andy Weir's 'The Egg'. What if every single person in human history was you?" , category : "Science & Tech" , ambient : "rgba(20, 184, 166, 0.35)" , duration : "08:04" , avatar : "K" } , { id : "aircAruvnKk" , title : "But what is a neural network? | Deep learning, Chapter 1" , channel : "3Blue1Brown" , subs : "6.4M subscribers" , views : "17,800,000 views • 6 years ago" , desc : "Visual explanation of neural networks, weights, biases, and gradient descent mathematically visualized." , category : "Coding & AI" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "19:13" , avatar : "3" } , { id : "7wnove7K-ZQ" , title : "Python Tutorial for Beginners - Full Course [2025]" , channel : "Programming with Mosh" , subs : "4.1M subscribers" , views : "38,500,000 views • 5 years ago" , desc : "Learn Python programming from scratch. Variables, loops, functions, lists, dictionaries, classes, and real projects." , category : "Coding & AI" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "6:14:07" , avatar : "P" } , { id : "heWDNn35X2w" , title : "How Special Relativity Makes Magnets Work" , channel : "Veritasium" , subs : "16.1M subscribers" , views : "14,800,000 views • 10 years ago" , desc : "Magnetism is purely a relativistic side-effect of electrostatic forces when electrons move through a wire." , category : "Science & Tech" , ambient : "rgba(16, 185, 129, 0.35)" , duration : "04:19" , avatar : "V" } , { id : "eaW0tYpxyp0" , title : "Elden Ring - Official Gameplay Reveal Trailer" , channel : "Bandai Namco" , subs : "2.1M subscribers" , views : "16,400,000 views • 3 years ago" , desc : "Rise, Tarnished, and be guided by grace to brandish the power of the Elden Ring and become an Elden Lord in the Lands Between." , category : "Gaming" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "02:58" , avatar : "B" } , { id : "kJQP7kiw5Fk" , title : "Luis Fonsi - Despacito ft. Daddy Yankee" , channel : "Luis Fonsi" , subs : "32.1M subscribers" , views : "8,500,000,000 views • 7 years ago" , desc : "Despacito official music video by Luis Fonsi & Daddy Yankee. 8.5+ Billion views worldwide." , category : "Lo-Fi Music" , ambient : "rgba(239, 68, 68, 0.35)" , duration : "04:41" , avatar : "D" } , { id : "jNQXAC9IVRw" , title : "Me at the zoo (First YouTube Video in History)" , channel : "jawed" , subs : "4.8M subscribers" , views : "325,000,000 views • 19 years ago" , desc : "The first video on YouTube, uploaded on April 23, 2005. Recorded at the San Diego Zoo by Jawed Karim." , category : "All" , ambient : "rgba(34, 197, 94, 0.35)" , duration : "00:19" , avatar : "J" } ];

const shortsList = [ { title : "🔥 3 Insane Java 21 Performance Hacks you never knew existed! #Java #SpringBoot #Dev" , author : "@SpandanPrayas" , audio : "🎵 Spandan Original Audio • High Performance Remix" , views : "3.2M" , bg : "linear-gradient(180deg, #1e3a8a, #0f172a)" , vid : "gJrjgg1KVL4" } , { title : "🚀 Write Clean Code in English with Enlang! Zero boilerplate #Enlang #Compilers" , author : "@EnlangOfficial" , audio : "🎵 Enlang Sound Studio • Compiler Waves" , views : "1.8M" , bg : "linear-gradient(180deg, #047857, #064e3b)" , vid : "dQw4w9WgXcQ" } , { title : "🌐 Cloudflare Workers under 1ms Latency around the world #Edge #Cloudflare" , author : "@EdgeArchitects" , audio : "🎵 Ultra Edge Lo-Fi Beat" , views : "950K" , bg : "linear-gradient(180deg, #6d28d9, #2e1065)" , vid : "8aGhZQkoFbQ" } , { title : "✨ Raymarched Black Hole in 30 Lines of WebGL Shader #Threejs #GLSL" , author : "@CreativeCoding3D" , audio : "🎵 Cosmic Raymarch Audio" , views : "2.4M" , bg : "linear-gradient(180deg, #b45309, #451a03)" , vid : "Q7AOvWpIVHU" } , { title : "🎮 GTA 6 Vice City Graphics will blow your mind #GTA6 #Rockstar #Gaming" , author : "@RockstarGames" , audio : "🎵 Love Is A Long Road • Tom Petty" , views : "12.4M" , bg : "linear-gradient(180deg, #831843, #500724)" , vid : "L_LUpnjgPso" } , { title : "⚡ $456,000 Squid Game Behind The Scenes Secret #MrBeast #Challenge" , author : "@MrBeast" , audio : "🎵 Squid Game Official Theme" , views : "45.1M" , bg : "linear-gradient(180deg, #991b1b, #450a0a)" , vid : "0e3GPea1Tyg" } , { title : "🍎 Vision Pro Eye Tracking is Magic #Apple #VisionPro #Tech" , author : "@MKBHD" , audio : "🎵 MKBHD Intro Sound" , views : "8.9M" , bg : "linear-gradient(180deg, #374151, #111827)" , vid : "1-2J_aFkYlY" } , { title : "💣 How Glitter Bomb 5.0 Captures 360 Video #MarkRober #Engineering" , author : "@MarkRober" , audio : "🎵 Mission Impossible Remix" , views : "14.2M" , bg : "linear-gradient(180deg, #d97706, #78350f)" , vid : "bhiE28f9q4c" } ];

function showToast(message) {
  let toastEl = document.getElementById ( "yt-toast" );
  if (toastEl != null) {
    toastEl.textContent = message;
    toastEl.style.display = "block";
    setTimeout(function() {
      let t2 = document.getElementById ( "yt-toast" );
      if (t2 != null) {
        t2.style.display = "none";
      }
    }, 3000);
  }
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
  if (ifr != null) {
    if (ifr.contentWindow != null) {
      ifr.contentWindow.postMessage ( JSON.stringify ( { event : "command" , func : funcName , args : args || [ ] } ) , "*" );
    }
  }
}

function sendShortsCmd(funcName, args) {
  let ifr = document.getElementById ( "shorts-iframe" );
  if (ifr != null) {
    if (ifr.contentWindow != null) {
      ifr.contentWindow.postMessage ( JSON.stringify ( { event : "command" , func : funcName , args : args || [ ] } ) , "*" );
    }
  }
}

function hideAllViews() {
  let pFeed = document.getElementById ( "view-feed" );
  let pShorts = document.getElementById ( "view-shorts" );
  let pWatch = document.getElementById ( "view-watch" );
  let pSubs = document.getElementById ( "view-subs" );
  let pLib = document.getElementById ( "view-library" );
  let pTrend = document.getElementById ( "view-trending" );
  if (pFeed != null) {
    pFeed.style.display = "none";
  }
  if (pShorts != null) {
    pShorts.style.display = "none";
  }
  if (pWatch != null) {
    pWatch.style.display = "none";
  }
  if (pSubs != null) {
    pSubs.style.display = "none";
  }
  if (pLib != null) {
    pLib.style.display = "none";
  }
  if (pTrend != null) {
    pTrend.style.display = "none";
  }
}

function openHomeFeed() {
  hideAllViews();
  let pFeed = document.getElementById ( "view-feed" );
  if (pFeed != null) {
    pFeed.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderGrid ( masterCatalog );
  showToast("🏠 YouTube Home Feed");
}

function openShortsReel() {
  hideAllViews();
  let pShorts = document.getElementById ( "view-shorts" );
  if (pShorts != null) {
    pShorts.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderActiveShort ( 0 );
  showToast("⚡ YouTube Shorts Reel active");
}

function openSubsFeed() {
  hideAllViews();
  let pSubs = document.getElementById ( "view-subs" );
  if (pSubs != null) {
    pSubs.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderSubsGrid ( );
  showToast("📺 Subscriptions Feed");
}

function openLibrary() {
  hideAllViews();
  let pLib = document.getElementById ( "view-library" );
  if (pLib != null) {
    pLib.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderLibraryView ( );
  showToast("📁 Library & Watch History");
}

function openTrending() {
  hideAllViews();
  let pTrend = document.getElementById ( "view-trending" );
  if (pTrend != null) {
    pTrend.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderTrendingList ( );
  showToast("🔥 Trending on YouTube");
}

function renderActiveShort(idx) {
  currentShortIndex = idx;
  let s = shortsList [ idx ];
  if (s == null) {
    return;
  }
  let sBox = document.getElementById ( "shorts-active-box" );
  if (sBox != null) {
    sBox.style.background = s.bg;
  }
  let sIfr = document.getElementById ( "shorts-iframe" );
  if (sIfr != null) {
    sIfr.src = "https://www.youtube-nocookie.com/embed/" + s.vid + "?autoplay=1&enablejsapi=1&controls=0&loop=1";
  }
  let sDesc = document.getElementById ( "shorts-title-text" );
  if (sDesc != null) {
    sDesc.textContent = s.title;
  }
  let sAuthor = document.getElementById ( "shorts-author-name" );
  if (sAuthor != null) {
    sAuthor.textContent = s.author;
  }
  let sAudio = document.getElementById ( "shorts-audio-text" );
  if (sAudio != null) {
    sAudio.textContent = s.audio;
  }
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
      btn.onclick = function ( ) { set playSeconds = ch.sec sendPlayerCmd ( "seekTo" , [ ch.sec , true ] ) showToast ( "📍 Jumped to " + ch.label ) };
      cWrapper.appendChild ( btn );
    }
  }
}

function renderCommentsForVideo(vidId, title, channel) {
  let cContainer = document.getElementById ( "comment-list-container" );
  if (cContainer != null) {
    cContainer.innerHTML = "";
    let mockList = [ { author : channel , badge : "Creator ✓" , time : "1 day ago" , : "Source code repository & full timestamps are in the description! Drop your questions below and I will answer every single one." , likes : "428" , isPinned : true } , { author : "@AlexDeveloper" , badge : "" , time : "3 hours ago" , : "The explanation in this video is crystal clear! Best video on this topic on YouTube." , likes : "94" , isPinned : false } , { author : "@SarahCode" , badge : "" , time : "6 hours ago" , : "Enlang compiling directly to native HTML, CSS, JS with zero boilerplate is revolutionary." , likes : "67" , isPinned : false } , { author : "@DevMaster" , badge : "" , time : "1 day ago" , : "Re-watched this 3 times already. The production quality and pacing is top tier." , likes : "32" , isPinned : false } ];
    for (const c of mockList) {
      let card = document.createElement ( "div" );
      card.className = c.isPinned ? "comment-card pinned" : "comment-card";
      let pinnedHeader = c.isPinned ? "<span class='pinned-tag'>📌 Pinned by " + channel + "</span>" : "";
      card.innerHTML = "<div class='user-avatar-sm' style='background: #3b82f6;'>" + c.author.charAt ( 1 ) .toUpperCase ( ) + "</div><div class='comment-body'>" + pinnedHeader + "<div class='comment-author-row'><span class='comment-author'>" + c.author + "</span>" + ( c.badge ? "<span class='creator-pill'>" + c.badge + "</span>" : "" ) + "<span class='comment-time'>" + c.time + "</span></div><p class='comment-msg'>" + c.text + "</p><div class='comment-reactions'><button class='btn-react'>👍 " + c.likes + "</button><button class='btn-react'>👎</button><button class='btn-react'>Reply</button></div></div>";
      cContainer.appendChild ( card );
    }
  }
}

function openWatchPage(vidId, title, channel, views, desc, avatar) {
  currentVideoId = vidId;
  let targetTitle = title || "YouTube Stream Video";
  let targetChannel = channel || "YouTube Creator";
  let targetViews = views || "100K views";
  let targetDesc = desc || ( "Watch " + targetTitle + " streaming in high definition on YouTube." );
  let targetAvatar = avatar || ( targetChannel.charAt ( 0 ) || "Y" );
  (document.getElementById("watch-video-title") || document.querySelector("watch-video-title")).textContent = targetTitle;
  (document.getElementById("watch-channel-name") || document.querySelector("watch-channel-name")).textContent = targetChannel;
  (document.getElementById("watch-desc-text") || document.querySelector("watch-desc-text")).textContent = targetDesc;
  (document.getElementById("desc-stats") || document.querySelector("desc-stats")).textContent = targetViews + " • Premiered recently";
  (document.getElementById("watch-channel-avatar") || document.querySelector("watch-channel-avatar")).textContent = targetAvatar;
  let shareInput = document.getElementById ( "share-link-input" );
  if (shareInput != null) {
    shareInput.value = "https://youtu.be/" + vidId;
  }
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
  hideAllViews();
  let pWatch = document.getElementById ( "view-watch" );
  if (pWatch != null) {
    pWatch.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderRecommendations ( vidId );
  renderChaptersForVideo ( vidId , targetTitle );
  renderCommentsForVideo ( vidId , targetTitle , targetChannel );
  showToast("▶ Playing: " + targetTitle);
}

function renderRecommendations(activeVidId) {
  let recList = document.getElementById ( "rec-list-container" );
  if (recList != null) {
    recList.innerHTML = "";
    let others = masterCatalog.filter ( function ( item ) { return item.id !== activeVidId } );
    for (const item of others) {
      let card = document.createElement ( "div" );
      card.className = "rec-card";
      card.innerHTML = "<div class='rec-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='rec-details'><h4 class='rec-title'>" + item.title + "</h4><p class='rec-channel'>" + item.channel + " ✓</p><p class='rec-stats'>" + item.views + "</p></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      recList.appendChild ( card );
    }
  }
}

function renderSubsGrid() {
  let sRow = document.getElementById ( "subs-channels-row" );
  if (sRow != null) {
    sRow.innerHTML = "";
    let subChannels = [ { name : "Spandan Prayas" , avatar : "S" , bg : "#8b5cf6" } , { name : "MrBeast" , avatar : "M" , bg : "#ef4444" } , { name : "JavaScript Mastery" , avatar : "J" , bg : "#10b981" } , { name : "ByteByteGo" , avatar : "B" , bg : "#f59e0b" } , { name : "FreeCodeCamp" , avatar : "F" , bg : "#06b6d4" } , { name : "Veritasium" , avatar : "V" , bg : "#3b82f6" } , { name : "MKBHD" , avatar : "M" , bg : "#6366f1" } , { name : "Rockstar Games" , avatar : "R" , bg : "#ec4899" } ];
    for (const sc of subChannels) {
      let bubble = document.createElement ( "div" );
      bubble.className = "channel-story-bubble";
      bubble.innerHTML = "<div class='channel-story-avatar' style='background:" + sc.bg + ";'>" + sc.avatar + "</div><span class='channel-story-name'>" + sc.name + "</span>";
      bubble.onclick = function ( ) { searchLiveYouTube ( sc.name ) };
      sRow.appendChild ( bubble );
    }
  }
  let sGrid = document.getElementById ( "subs-grid-container" );
  if (sGrid != null) {
    sGrid.innerHTML = "";
    for (const item of masterCatalog) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'><div class='channel-avatar'>" + item.avatar + "</div><div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      sGrid.appendChild ( card );
    }
  }
}

function renderLibraryView() {
  let hContainer = document.getElementById ( "history-list-container" );
  if (hContainer != null) {
    hContainer.innerHTML = "";
    let histItems = masterCatalog.slice ( 0 , 6 );
    for (const item of histItems) {
      let card = document.createElement ( "div" );
      card.className = "history-card";
      card.innerHTML = "<div class='history-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='history-details'><h3 class='history-title'>" + item.title + "</h3><p class='history-sub'>" + item.channel + " • " + item.views + "</p><p class='history-time'>Watched recently</p></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      hContainer.appendChild ( card );
    }
  }
  let wlContainer = document.getElementById ( "watch-later-container" );
  if (wlContainer != null) {
    wlContainer.innerHTML = "";
    let wlItems = masterCatalog.slice ( 2 , 6 );
    for (const item of wlItems) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'><div class='channel-avatar'>" + item.avatar + "</div><div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      wlContainer.appendChild ( card );
    }
  }
  let lkContainer = document.getElementById ( "liked-videos-container" );
  if (lkContainer != null) {
    lkContainer.innerHTML = "";
    let lkItems = masterCatalog.slice ( 0 , 4 );
    for (const item of lkItems) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'><div class='channel-avatar'>" + item.avatar + "</div><div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      lkContainer.appendChild ( card );
    }
  }
}

function renderTrendingList() {
  let tContainer = document.getElementById ( "trending-list-container" );
  if (tContainer != null) {
    tContainer.innerHTML = "";
    let rank = 1;
    for (const item of masterCatalog) {
      let card = document.createElement ( "div" );
      card.className = "trending-card";
      card.innerHTML = "<div class='trending-rank-badge'>#" + rank + "</div><div class='history-thumb'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='history-details'><h3 class='history-title'>" + item.title + "</h3><p class='history-sub'>" + item.channel + " ✓ • " + item.views + "</p><p class='desc-content' style='color:#aaa; font-size:12px;'>" + item.desc.substring ( 0 , 100 ) + "...</p></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      tContainer.appendChild ( card );
      rank = rank + 1;
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
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'><div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div><div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      grid.appendChild ( card );
    }
  }
}

function appendMoreFeedVideos() {
  let grid = document.getElementById ( "video-grid-container" );
  if (grid != null) {
    feedRenderCount = feedRenderCount + 1;
    let pool = masterCatalog;
    if (currentCategoryFilter != "All") {
      let filtered = masterCatalog.filter ( function ( item ) { return item.category == currentCategoryFilter } );
      if (filtered.length > 0) {
        pool = filtered;
      }
    }
    for (const item of pool) {
      let card = document.createElement ( "div" );
      card.className = "video-card";
      card.innerHTML = "<div class='thumbnail-wrapper'><img src='https://i.ytimg.com/vi/" + item.id + "/hqdefault.jpg' class='thumb-real-img' /><span class='duration-badge'>" + item.duration + "</span></div><div class='video-meta'><div class='channel-avatar'>" + ( item.avatar || item.channel.charAt ( 0 ) ) + "</div><div class='video-details'><h3 class='video-title'>" + item.title + "</h3><p class='video-channel'>" + item.channel + " ✓</p><p class='video-stats'>" + item.views + "</p></div></div>";
      card.onclick = function ( ) { openWatchPage ( item.id , item.title , item.channel , item.views , item.desc , item.avatar ) };
      grid.appendChild ( card );
    }
    showToast("✨ Loaded " + pool.length + " more real YouTube videos into feed!");
  }
}

function filterByCategory(categoryName) {
  currentCategoryFilter = categoryName;
  let allChips = document.querySelectorAll ( ".chip" );
  allChips.forEach ( function ( chip ) { chip.classList.remove ( "active" ) } );
  let statusTitle = document.getElementById ( "live-status-title" );
  let statusDesc = document.getElementById ( "live-status-desc" );
  if (categoryName == "All") {
    let allChipElement = document.getElementById ( "chip-all" );
    if (allChipElement != null) {
      allChipElement.classList.add ( "active" );
    }
    renderGrid ( masterCatalog );
    if (statusTitle != null) {
      statusTitle.textContent = "🔴 Infinite YouTube Feed";
    }
    if (statusDesc != null) {
      statusDesc.textContent = "Displaying all " + masterCatalog.length + " live videos across all genres • Infinite Scroll active";
    }
  }
  else {
    let filtered = masterCatalog.filter ( function ( item ) { return item.category == categoryName } );
    if (filtered.length == 0) {
      filtered = masterCatalog;
    }
    renderGrid ( filtered );
    if (statusTitle != null) {
      statusTitle.textContent = "🔴 Category: " + categoryName;
    }
    if (statusDesc != null) {
      statusDesc.textContent = "Displaying " + filtered.length + " curated live YouTube videos for " + categoryName;
    }
  }
  showToast("Showing category: " + categoryName);
}

function searchLiveYouTube(query) {
  let q = query.trim ( );
  if (q == "") {
    showToast("Please type a search query or paste a YouTube URL");
    return;
  }
  showToast("🔍 Searching for: " + q);
  let directVidId = "";
  if (q.length == 11 && not q.includes ( " " )) {
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
  let matches = masterCatalog.filter ( function ( v ) { return v.title.toLowerCase ( ) .includes ( qLow ) || v.channel.toLowerCase ( ) .includes ( qLow ) || v.desc.toLowerCase ( ) .includes ( qLow ) || v.category.toLowerCase ( ) .includes ( qLow ) } );
  openHomeFeed();
  let statusTitle = document.getElementById ( "live-status-title" );
  let statusDesc = document.getElementById ( "live-status-desc" );
  if (matches.length > 0) {
    renderGrid ( matches );
    if (statusTitle != null) {
      statusTitle.textContent = "🔍 Search Results for: " + q;
    }
    if (statusDesc != null) {
      statusDesc.textContent = "Found " + matches.length + " matching real YouTube videos";
    }
    showToast("Found " + matches.length + " videos matching '" + q + "'");
  }
  else {
    renderGrid ( masterCatalog );
    if (statusTitle != null) {
      statusTitle.textContent = "🔍 Showing all results for: " + q;
    }
    if (statusDesc != null) {
      statusDesc.textContent = "Showing top trending videos for: " + q;
    }
    showToast("Showing top videos for '" + q + "'");
  }
}

renderGrid ( masterCatalog );

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
      appendMoreFeedVideos();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-sidebar-toggle") || document.querySelector("btn-sidebar-toggle"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sb = document.getElementById ( "yt-sidebar" );
      if (sb != null) {
        if (sb.style.display == "none") {
          sb.style.display = "flex";
        }
        else {
          sb.style.display = "none";
        }
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-create") || document.querySelector("btn-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mCreate = document.getElementById ( "modal-create" );
      if (mCreate != null) {
        mCreate.style.display = "flex";
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-create") || document.querySelector("btn-close-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mCreate = document.getElementById ( "modal-create" );
      if (mCreate != null) {
        mCreate.style.display = "none";
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-upload") || document.querySelector("btn-cancel-upload"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mCreate = document.getElementById ( "modal-create" );
      if (mCreate != null) {
        mCreate.style.display = "none";
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-publish-video") || document.querySelector("btn-publish-video"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let uTitle = (document.getElementById("upload-title-input") || document.querySelector("upload-title-input")).value;
      if (uTitle == "") {
        uTitle = "New Enlang Video by Spandan Prayas Patra";
      }
      let newV = { id : "gJrjgg1KVL4" , title : uTitle , channel : "Spandan Prayas Patra" , subs : "128K subscribers" , views : "1 view • Just now" , desc : "Newly uploaded creator video in Enlang." , category : "All" , duration : "12:30" , avatar : "S" };
      masterCatalog.unshift ( newV );
      openHomeFeed();
      let mCreate = document.getElementById ( "modal-create" );
      if (mCreate != null) {
        mCreate.style.display = "none";
      }
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
          dNotif.style.display = "none";
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
      showToast("🎙️ Listening... Say 'Spring Boot', 'MrBeast', or 'Gaming' to search");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("user-profile") || document.querySelector("user-profile"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("👤 Signed in as Spandan Prayas Patra (Creator)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-search") || document.querySelector("btn-search"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let q = (document.getElementById("search-input") || document.querySelector("search-input")).value;
      searchLiveYouTube(q);
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
      let nextItem = masterCatalog [ 1 ];
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
  const targetEl = (document.getElementById("btn-player-cc") || document.querySelector("btn-player-cc"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("📝 Closed Captions: English (Auto-generated)");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-player-quality") || document.querySelector("btn-player-quality"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      showToast("⚙️ Stream Quality: 1080p60 Full HD");
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
        sBtn.innerText = "Subscribe";
        sBtn.style.background = "#ffffff";
        sBtn.style.color = "#0f0f0f";
        showToast("Unsubscribed from Creator");
      }
      else {
        isSubscribed = true;
        sBtn.innerText = "Subscribed ✓";
        sBtn.style.background = "#272727";
        sBtn.style.color = "#aaaaaa";
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
        lBtn.innerText = "👍 142K";
        lBtn.style.color = "#f1f1f1";
      }
      else {
        isLiked = true;
        lBtn.innerText = "👍 142.1K";
        lBtn.style.color = "#3ea6ff";
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
  const targetEl = (document.getElementById("btn-share") || document.querySelector("btn-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mShare = document.getElementById ( "modal-share" );
      if (mShare != null) {
        mShare.style.display = "flex";
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-share") || document.querySelector("btn-close-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mShare = document.getElementById ( "modal-share" );
      if (mShare != null) {
        mShare.style.display = "none";
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-copy-share") || document.querySelector("btn-copy-share"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let mShare = document.getElementById ( "modal-share" );
      if (mShare != null) {
        mShare.style.display = "none";
      }
      showToast("🔗 Link copied to clipboard!");
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
      let nextIdx = ( currentShortIndex + 1 ) % shortsList.length;
      renderActiveShort ( nextIdx );
      showToast("Next Short loaded");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-prev") || document.querySelector("btn-shorts-prev"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let prevIdx = ( currentShortIndex - 1 + shortsList.length ) % shortsList.length;
      renderActiveShort ( prevIdx );
      showToast("Previous Short loaded");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-shorts-like") || document.querySelector("btn-shorts-like"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let sLike = document.getElementById ( "shorts-like-count" );
      if (isShortsLiked == true) {
        isShortsLiked = false;
        if (sLike != null) {
          sLike.textContent = "245K";
        }
      }
      else {
        isShortsLiked = true;
        if (sLike != null) {
          sLike.textContent = "245.1K";
        }
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
          sBtn.innerText = "Subscribed ✓";
          sBtn.style.background = "#272727";
          sBtn.style.color = "#aaaaaa";
          showToast("Subscribed to Creator!");
        }
        else {
          sBtn.innerText = "Subscribe";
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
  let cText = (document.getElementById("new-comment-input") || document.querySelector("new-comment-input")).value;
  if (cText == "") {
    showToast("Please type a comment before posting");
    return;
  }
  let cContainer = document.getElementById ( "comment-list-container" );
  if (cContainer != null) {
    let cCard = document.createElement ( "div" );
    cCard.className = "comment-card";
    cCard.innerHTML = "<div class='user-avatar-sm' style='background: #8b5cf6;'>S</div><div class='comment-body'><div class='comment-author-row'><span class='comment-author'>@SpandanPrayas (You)</span><span class='comment-time'>Just now</span></div><p class='comment-msg'>" + cText + "</p><div class='comment-reactions'><button class='btn-react'>👍 1</button><button class='btn-react'>👎</button><button class='btn-react'>Reply</button></div></div>";
    cContainer.prepend ( cCard );
    (document.getElementById("new-comment-input") || document.querySelector("new-comment-input")).value = "";
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
      let hContainer = document.getElementById ( "history-list-container" );
      if (hContainer != null) {
        hContainer.innerHTML = "<p style='color: #888; padding: 20px;'>Watch history is empty.</p>";
      }
      showToast("🗑️ Watch History Cleared");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-theme") || document.querySelector("btn-theme"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let b = document.body;
      let tBtn = document.getElementById ( "btn-theme" );
      if (b.classList.contains ( "light-theme" )) {
        b.classList.remove ( "light-theme" );
        if (tBtn != null) {
          (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☾";
        }
        showToast("Switched to YouTube Dark Mode");
      }
      else {
        b.classList.add ( "light-theme" );
        if (tBtn != null) {
          (document.getElementById("btn-theme") || document.querySelector("btn-theme")).textContent = "☼";
        }
        showToast("Switched to YouTube Light Mode");
      }
    });
  }
})();