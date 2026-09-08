import os

try:
    import fitz # PyMuPDF
    doc = fitz.open(r"d:\enlangg\book\enlngdb_manual.pdf")
    print(f"PyMuPDF: Successfully opened PDF with {len(doc)} pages.")
    
    # Render pages: 1 (cover), 4 (TOC), 25 (Chapter early), 150 (Mid chapter), 350 (Case study), 500 (Appendices)
    pages_to_render = [1, 4, 25, 150, 350, 500]
    out_dir = r"C:\Users\spand\.gemini\antigravity-ide\brain\e6e86c22-5969-4828-8ab8-398f2b355fa4"
    
    for p_num in pages_to_render:
        if p_num <= len(doc):
            page = doc[p_num - 1]
            pix = page.get_pixmap(dpi=150)
            img_path = os.path.join(out_dir, f"manual_500p_page_{p_num}.png")
            pix.save(img_path)
            print(f"Saved page {p_num} preview to {img_path}")
            
except ImportError:
    print("PyMuPDF not installed, checking pypdf...")
    import pypdf
    reader = pypdf.PdfReader(r"d:\enlangg\book\enlngdb_manual.pdf")
    print(f"pypdf: Total pages: {len(reader.pages)}")
    for p_num in [0, 3, 24, 149, 349, 499]:
        if p_num < len(reader.pages):
            text = reader.pages[p_num].extract_text()
            print(f"--- Page {p_num+1} Sample Text (first 100 chars): {repr(text[:100])}")
