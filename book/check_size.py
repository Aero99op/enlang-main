import os
size = os.path.getsize(r"d:\enlangg\book\enlngdb_manual.pdf")
print(f"File Size: {size / (1024*1024):.2f} MB ({size:,} bytes)")
