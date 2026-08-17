import sys
sys.path.insert(0, ".")
from enlgs.compiler import compile_enlgs_file

test_code = '''in script:
    to do renderGrid with items:
        create htmlMarkup as ""
        for each item in items:
            create cardHtml as "<div class='video-card' id='card-" + item.id + "'>...</div>"
            set htmlMarkup = htmlMarkup + cardHtml
        set html of "video-grid-container" to htmlMarkup

    async to do fetchLiveYouTubeFromWeb with query:
        set text of "live-status-title" to "Fetching..."
        create targetUrl as "https://invidious.flokinet.to/api/v1/search?q=" + encodeURIComponent(query)
        create response as await fetch(targetUrl)
        create data as await response.json()
        show data
'''

with open("scratch/test_pure_syntax.enlgs", "w", encoding="utf-8") as f:
    f.write(test_code)

js = compile_enlgs_file("scratch/test_pure_syntax.enlgs")
print("Compiled JS output:\n", js)
