html = open('portfolio.html', encoding='utf-8').read()
js = html.split('<script>')[2].split('</script>')[0]
open('scratch/temp.js', 'w', encoding='utf-8').write(js)
