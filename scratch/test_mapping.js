const https = require('https');

function testLiveSearch(query) {
    const url = `https://invidious.flokinet.to/api/v1/search?q=${encodeURIComponent(query)}&type=video`;
    https.get(url, { headers: { 'User-Agent': 'Mozilla/5.0' } }, (res) => {
        let raw = '';
        res.on('data', chunk => raw += chunk);
        res.on('end', () => {
            try {
                const data = JSON.parse(raw);
                console.log(`Received ${data.length} live items for query '${query}':`);
                const mapped = data.map(item => ({
                    id: item.videoId,
                    title: item.title,
                    channel: item.author,
                    views: (item.viewCount ? (item.viewCount > 1000000 ? (item.viewCount/1000000).toFixed(1) + "M views" : (item.viewCount/1000).toFixed(0) + "K views") : "100K views") + " • " + (item.publishedText || "Recently"),
                    duration: item.lengthSeconds ? `${Math.floor(item.lengthSeconds/60)}:${(item.lengthSeconds%60 < 10 ? '0' : '') + (item.lengthSeconds%60)}` : "10:00",
                    desc: item.description || `Watch ${item.title} streaming live on YouTube.`,
                    avatar: (item.author && item.author.charAt(0)) || "Y",
                    category: "All"
                }));
                console.log("First 3 mapped cards:");
                console.log(JSON.stringify(mapped.slice(0, 3), null, 2));
            } catch (e) {
                console.error("Parse error:", e.message);
            }
        });
    }).on('error', e => console.error("Request error:", e.message));
}

testLiveSearch("snax gaming");
