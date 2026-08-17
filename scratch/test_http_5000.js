const http = require('http');

http.get('http://localhost:5000/youtube.html', res => {
    console.log('HTTP Status on port 5000:', res.statusCode);
    let data = '';
    res.on('data', chunk => data += chunk);
    res.on('end', () => {
        console.log('Received HTML size:', data.length);
        console.log('Contains searchLiveYouTube:', data.includes('searchLiveYouTube'));
        console.log('Contains card-gJrjgg1KVL4:', data.includes('card-gJrjgg1KVL4') || data.includes('gJrjgg1KVL4'));
        console.log('Contains dropdown-profile:', data.includes('dropdown-profile'));
    });
});
