const https = require('https');

function testLiveComments(vidId) {
    const url = `https://invidious.flokinet.to/api/v1/comments/${vidId}`;
    https.get(url, { headers: { 'User-Agent': 'Mozilla/5.0' } }, (res) => {
        let raw = '';
        res.on('data', chunk => raw += chunk);
        res.on('end', () => {
            try {
                const data = JSON.parse(raw);
                const comments = (data.comments || []).map(c => {
                    const avatarUrl = (c.authorThumbnails && c.authorThumbnails.length > 0) ? c.authorThumbnails[0].url : '';
                    return {
                        author: c.author || 'Viewer',
                        avatar: avatarUrl,
                        time: c.publishedText || 'Recently',
                        msg: c.content || '',
                        likes: c.likeCount ? (c.likeCount > 1000 ? (c.likeCount/1000).toFixed(1) + 'K' : c.likeCount) : '0',
                        isPinned: c.isPinned || false
                    };
                });
                console.log(`Successfully mapped ${comments.length} live YouTube comments for Snax Gaming (${vidId}):`);
                console.log(JSON.stringify(comments.slice(0, 3), null, 2));
            } catch (e) {
                console.error("Parse error:", e.message);
            }
        });
    }).on('error', e => console.error("Fetch error:", e.message));
}

testLiveComments("X58t82y5bz4");
