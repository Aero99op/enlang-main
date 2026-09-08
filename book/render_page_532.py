import pymupdf, os
doc = pymupdf.open(r"d:\enlangg\book\enlngdb_manual.pdf")
page = doc[531] # Page 532 (0-indexed 531)
pix = page.get_pixmap(dpi=150)
img_path = r"C:\Users\spand\.gemini\antigravity-ide\brain\e6e86c22-5969-4828-8ab8-398f2b355fa4\manual_500p_page_532.png"
pix.save(img_path)
print(f"Saved final page 532 to {img_path}")
