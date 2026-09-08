import sys
sys.path.insert(0, r"d:\enlangg\book")
import book_chapters_data
print(f"Verified: {len(book_chapters_data.CHAPTERS_DATA)} chapters in book_chapters_data.py!")
print(f"First Chapter: {book_chapters_data.CHAPTERS_DATA[0][1]}")
print(f"Last Chapter: {book_chapters_data.CHAPTERS_DATA[-1][1]}")
