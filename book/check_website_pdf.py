import os
size = os.path.getsize(r"d:\enlangg\website\enlngdb_manual.pdf")
print(f"Website PDF File Size: {size / (1024*1024):.2f} MB ({size:,} bytes)")
