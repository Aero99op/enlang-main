import sys
sys.path.insert(0, ".")
from enlgd.compiler import compile_enlgd_file

css = compile_enlgd_file("youtube.enlgd")
with open("scratch/compiled_youtube.css", "w", encoding="utf-8") as f:
    f.write(css)

print(f"Compiled CSS length: {len(css)} bytes")
print("Contains .video-meta:", ".video-meta" in css)
print("Contains .channel-avatar:", ".channel-avatar" in css)
print("Contains .video-details:", ".video-details" in css)
