import pypdf
reader = pypdf.PdfReader('d:/enlangg/book/enlngdb_manual.pdf')
print(f"Total Pages: {len(reader.pages)}")
