const https = require('https');

function fetchYouTubeLive(query) {
    const url = `https://api.allorigins.win/raw?url=${encodeURIComponent(`https://www.youtube.com/results?search_query=${encodeURIComponent(query)}`)}`;
    console.log('Fetching live YouTube search via proxy:', url);
    https.get(url, (res) => {
        let html = '';
        res.on('data', chunk => html += chunk);
        res.on('end', () => {
            console.log('Received HTML size:', html.length);
            const match = html.match(/var ytInitialData = ({.*?});<\/script>/);
            if (match) {
                try {
                    const data = JSON.parse(match[1]);
                    const contents = data.contents.twoColumnSearchResultsRenderer.primaryContents.sectionListRenderer.contents;
                    const videos = [];
                    for (const sec of contents) {
                        if (sec.itemSectionRenderer && sec.itemSectionRenderer.contents) {
                            for (const item of sec.itemSectionRenderer.contents) {
                                if (item.videoRenderer) {
                                    const vr = item.videoRenderer;
                                    const vid = vr.videoId;
                                    const title = vr.title && vr.title.runs ? vr.title.runs[0].text : 'Video';
                                    const author = vr.ownerText && vr.ownerText.runs ? vr.ownerText.runs[0].text : 'Channel';
                                    const duration = (vr.lengthText && vr.lengthText.simpleText) ? vr.lengthText.simpleText : '10:00';
                                    const views = (vr.viewCountText && vr.viewCountText.simpleText) ? vr.viewCountText.simpleText : '100K views';
                                    videos.push({ id: vid, title, channel: author, duration, views, avatar: author.charAt(0) });
                                }
                            }
                        }
                    }
                    console.log(`SUCCESS! Extracted ${videos.length} live YouTube videos for query '${query}':`);
                    videos.slice(0, 5).forEach(v => {
                        console.log(` - [${v.id}] ${v.title} (${v.channel}) [${v.duration}]`);
                    });
                } catch (e) {
                    console.error('Parse error:', e.message);
                }
            } else {
                console.log('ytInitialData not matched in HTML.');
            }
        });
    }).on('error', err => {
        console.error('Fetch error:', err.message);
    });
}

fetchYouTubeLive('snax gaming');
