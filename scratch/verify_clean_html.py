import re
html = open('portfolio.html', encoding='utf-8').read()
scripts = re.findall(r'<script[^>]*>', html)
styles = re.findall(r'<style[^>]*>', html)
print(f"Number of <script> tags: {len(scripts)}")
print(f"Number of <style> tags: {len(styles)}")
for i, s in enumerate(re.finditer(r'<script[^>]*>(.*?)</script>', html, re.DOTALL)):
    print(f"--- Script {i+1} length: {len(s.group(1))} ---")
    print(s.group(1)[:120].strip())
