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

console.log("▶ Initializing 1:1 YouTube Engine in Pure Enlang (.enlgs)...");

const masterCatalog = [ { id : "gJrjgg1KVL4" , title : "Spring Boot 3 Full Course 2025 - Beginner to Pro" , channel : "Spandan Prayas Patra" , subs : "128K subscribers" , views : "1,039,987 views • Premiered on Aug 15, 2026" , desc : "In this deep dive tutorial, Spandan Prayas Patra walks through architecting mission-critical Java 21 backend microservices, configuring HikariCP connection pools for Oracle & MySQL, and applying strict ACID transaction management." , category : "Java Backend" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "1:11:22" , avatar : "S" } , { id : "dQw4w9WgXcQ" , title : "Rick Astley - Never Gonna Give You Up (Official Music Video 4K Remaster)" , channel : "Rick Astley" , subs : "3.4M subscribers" , views : "1,520,840,112 views • 14 years ago" , desc : "The official video for 'Never Gonna Give You Up' by Rick Astley. 4K Remastered with studio high definition audio." , category : "Lo-Fi Music" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "03:33" , avatar : "R" } , { id : "8aGhZQkoFbQ" , title : "Next.js 15 & React 19 Full Stack Masterclass (App Router, Server Actions)" , channel : "JavaScript Mastery" , subs : "1.1M subscribers" , views : "489,420 views • 3 weeks ago" , desc : "Master full-stack Next.js with Server Actions, Edge Middleware, and Cloudflare Deployments with zero 500 errors." , category : "Next.js" , ambient : "rgba(16, 185, 129, 0.35)" , duration : "2:45:10" , avatar : "J" } , { id : "HXV3zeRR3h4" , title : "System Design for High Scale - 100 Million Active Users" , channel : "ByteByteGo" , subs : "1.9M subscribers" , views : "980,120 views • 1 month ago" , desc : "An insider look into database sharding, distributed caches, and load balancers at massive hyperscale." , category : "System Design" , ambient : "rgba(249, 115, 22, 0.35)" , duration : "18:24" , avatar : "B" } , { id : "Q7AOvWpIVHU" , title : "Three.js 3D Web Graphics & WebGL Shaders Masterclass" , channel : "Creative Coding 3D" , subs : "340K subscribers" , views : "340,900 views • 5 days ago" , desc : "Shader art tutorial implementing dual plasma rings and procedural celestial textures in WebGL." , category : "Coding & AI" , ambient : "rgba(99, 102, 241, 0.35)" , duration : "21:15" , avatar : "T" } , { id : "pTJJsmejUOQ" , title : "Flutter 3.24 & Dart Full Course for Cross-Platform Mobile Apps" , channel : "FreeCodeCamp" , subs : "9.8M subscribers" , views : "1,420,150 views • 2 months ago" , desc : "Build cross-platform iOS & Android mobile applications with clean architecture and state management." , category : "Enlang Stack" , ambient : "rgba(20, 184, 166, 0.35)" , duration : "4:32:00" , avatar : "F" } , { id : "fJ9rUzIMcZQ" , title : "Queen - Bohemian Rhapsody (Official Video Remastered)" , channel : "Queen Official" , subs : "18.2M subscribers" , views : "1,740,290,000 views • 15 years ago" , desc : "The official music video for Queen's iconic Bohemian Rhapsody, remastered in HD." , category : "Lo-Fi Music" , ambient : "rgba(168, 85, 247, 0.35)" , duration : "05:59" , avatar : "Q" } , { id : "kXYiU_JCYtU" , title : "Linkin Park - Numb (Official Music Video 4K)" , channel : "Linkin Park" , subs : "21.4M subscribers" , views : "2,210,000,000 views • 17 years ago" , desc : "'Numb' by Linkin Park from the album Meteora. 4K Ultra High Definition remastered." , category : "Lo-Fi Music" , ambient : "rgba(59, 130, 246, 0.35)" , duration : "03:07" , avatar : "L" } , { id : "L_LUpnjgPso" , title : "Grand Theft Auto VI Official Trailer 1 - 4K 60FPS" , channel : "Rockstar Games" , subs : "10.8M subscribers" , views : "215,900,000 views • 8 months ago" , desc : "Grand Theft Auto VI heads to the state of Leonida, home to the neon-soaked streets of Vice City." , category : "Gaming" , ambient : "rgba(236, 72, 153, 0.35)" , duration : "01:31" , avatar : "R" } , { id : "Un5SEJ8MyPc" , title : "Cyberpunk 2077: Phantom Liberty - Official Cinematic Trailer" , channel : "Cyberpunk 2077" , subs : "1.4M subscribers" , views : "14,200,000 views • 1 year ago" , desc : "Return to Night City in this spy-thriller expansion featuring Idris Elba as Solomon Reed." , category : "Gaming" , ambient : "rgba(234, 179, 8, 0.35)" , duration : "04:12" , avatar : "C" } , { id : "jNQXAC9IVRw" , title : "Me at the zoo (First YouTube Video in History)" , channel : "jawed" , subs : "4.8M subscribers" , views : "325,000,000 views • 19 years ago" , desc : "The first video on YouTube, uploaded on April 23, 2005. Recorded at the San Diego Zoo." , category : "All" , ambient : "rgba(34, 197, 94, 0.35)" , duration : "00:19" , avatar : "J" } , { id : "kJQP7kiw5Fk" , title : "Luis Fonsi - Despacito ft. Daddy Yankee" , channel : "Luis Fonsi" , subs : "32.1M subscribers" , views : "8,500,000,000 views • 7 years ago" , desc : "Despacito official video by Luis Fonsi & Daddy Yankee." , category : "Lo-Fi Music" , ambient : "rgba(239, 68, 68, 0.35)" , duration : "04:41" , avatar : "D" } ];

const shortsList = [ { title : "🔥 3 Insane Java 21 Performance Hacks you never knew existed! #Java #SpringBoot #Dev" , author : "@SpandanPrayas" , audio : "🎵 Spandan Original Audio • High Performance Remix" , views : "3.2M" , bg : "linear-gradient(180deg, #1e3a8a, #0f172a)" , vid : "gJrjgg1KVL4" } , { title : "🚀 Write Clean Code in English with Enlang! Zero boilerplate #Enlang #Compilers" , author : "@EnlangOfficial" , audio : "🎵 Enlang Sound Studio • Compiler Waves" , views : "1.8M" , bg : "linear-gradient(180deg, #047857, #064e3b)" , vid : "dQw4w9WgXcQ" } , { title : "🌐 Cloudflare Workers under 1ms Latency around the world #Edge #Cloudflare" , author : "@EdgeArchitects" , audio : "🎵 Ultra Edge Lo-Fi Beat" , views : "950K" , bg : "linear-gradient(180deg, #6d28d9, #2e1065)" , vid : "8aGhZQkoFbQ" } , { title : "✨ Raymarched Black Hole in 30 Lines of WebGL Shader #Threejs #GLSL" , author : "@CreativeCoding3D" , audio : "🎵 Cosmic Raymarch Audio" , views : "2.4M" , bg : "linear-gradient(180deg, #b45309, #451a03)" , vid : "Q7AOvWpIVHU" } , { title : "🎮 GTA 6 Vice City Graphics will blow your mind #GTA6 #Rockstar #Gaming" , author : "@RockstarGames" , audio : "🎵 Love Is A Long Road • Tom Petty" , views : "12.4M" , bg : "linear-gradient(180deg, #831843, #500724)" , vid : "L_LUpnjgPso" } ];

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
}

function openHomeFeed() {
  hideAllViews();
  let pFeed = document.getElementById ( "view-feed" );
  if (pFeed != null) {
    pFeed.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderGrid(masterCatalog);
  showToast("🏠 YouTube Home Feed");
}

function openShortsReel() {
  hideAllViews();
  let pShorts = document.getElementById ( "view-shorts" );
  if (pShorts != null) {
    pShorts.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderActiveShort(0);
  showToast("⚡ YouTube Shorts Reel active");
}

function openSubsFeed() {
  hideAllViews();
  let pSubs = document.getElementById ( "view-subs" );
  if (pSubs != null) {
    pSubs.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderSubsGrid();
  showToast("📺 Subscriptions Feed");
}

function openLibrary() {
  hideAllViews();
  let pLib = document.getElementById ( "view-library" );
  if (pLib != null) {
    pLib.style.display = "block";
  }
  window.scrollTo ( 0 , 0 );
  renderHistoryList();
  showToast("📁 Library & Watch History");
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
  renderRecommendations(vidId);
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
      card.onclick = function ( ) {;
      openWatchPage(item.id, item.title, item.channel, item.views, item.desc, item.avatar);
    }
    }
    recList.appendChild ( card );
  }
}