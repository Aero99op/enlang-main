const https = require('https');

function testSearchAvatars() {
    const url = `https://invidious.flokinet.to/api/v1/search?q=snax%20gaming&type=video`;
    https.get(url, { headers: { 'User-Agent': 'Mozilla/5.0' } }, (res) => {
        let raw = '';
        res.on('data', chunk => raw += chunk);
        res.on('end', () => {
            const data = JSON.parse(raw);
            const items = data.map(v => ({
                id: v.videoId,
                title: v.title,
                channel: v.author,
                avatar: (v.authorThumbnails && v.authorThumbnails.length > 0) ? v.authorThumbnails[v.authorThumbnails.length - 1].url : ''
            }));
            console.log("Mapped search avatars:", items.slice(0, 3));
        });
    });
}
testSearchAvatars();
