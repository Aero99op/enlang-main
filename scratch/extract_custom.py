import re
html = open('portfolio.html', encoding='utf-8').read()
custom_js = re.findall(r'<script[^>]*>(.*?)</script>', html, re.DOTALL)[1]
open('scratch/custom_js.js', 'w', encoding='utf-8').write(custom_js)
print("Saved scratch/custom_js.js, length:", len(custom_js))
