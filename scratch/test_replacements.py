import urllib.request

replacements = [
    ("M72t5o_O0J8", "System Design"),
    ("i53Gi_Ka38o", "System Design 2"),
    ("h4T_LlK1VE4", "Mark Rober"),
    ("dtp6bwt6htU", "MKBHD"),
    ("hdwaWVtf0hE", "MKBHD 2"),
    ("bHIhgxav9LY", "Veritasium"),
    ("5qap5aO4i9A", "Lofi Girl"),
    ("DWcJFNfaw9c", "Lofi Beats"),
    ("XqZsoesa55w", "Baby Shark"),
    ("9bZkp7q19f0", "Gangnam Style"),
    ("JGwWNGJdvx8", "Shape of You"),
    ("k85mRPqvMlE", "Crazy Frog"),
    ("RgKAFK5djSk", "See You Again"),
    ("W6NZfCO5SIk", "JavaScript Mosh"),
    ("bMknfKXIFA8", "React Course"),
    ("d1_JBMrrYw8", "Wukong Trailer"),
    ("T6sL_k16JzM", "MacBook Dave2D")
]

for vid, desc in replacements:
    url = f"https://i.ytimg.com/vi/{vid}/hqdefault.jpg"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=3) as resp:
            print(f"  [OK 200] {vid} -> {desc}")
    except Exception as e:
        print(f"  [FAIL] {vid} ({desc}): {e}")
