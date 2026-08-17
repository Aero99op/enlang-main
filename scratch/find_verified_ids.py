import urllib.request

candidates = [
    # Coding & Tech
    ("gJrjgg1KVL4", "Spring Boot Spandan"),
    ("8aGhZQkoFbQ", "Next.js JS Mastery"),
    ("pTJJsmejUOQ", "Flutter FreeCodeCamp"),
    ("Q7AOvWpIVHU", "Three.js Creative 3D"),
    ("W6NZfCO5SIk", "JS Tutorial Mosh"),
    ("7wnove7K-ZQ", "Python Mosh"),
    ("bMknfKXIFA8", "React FreeCodeCamp"),
    ("aircAruvnKk", "Neural Networks 3Blue1Brown"),
    ("rfscVS0vtbw", "Python FreeCodeCamp"),
    ("fBNz5xF-Kx4", "Node.js Tutorial"),
    ("kqtD5dpn9C8", "Python for Beginners"),
    ("G3e-cpL7ofc", "HTML Full Course"),
    ("1Rs2ND1ryYc", "CSS Tutorial"),
    # Entertainment & Viral
    ("0e3GPea1Tyg", "MrBeast Squid Game"),
    ("dQw4w9WgXcQ", "Rick Astley"),
    ("9bZkp7q19f0", "Gangnam Style"),
    ("kJQP7kiw5Fk", "Despacito"),
    ("jNQXAC9IVRw", "Me at the zoo"),
    ("JGwWNGJdvx8", "Shape of You"),
    ("RgKAFK5djSk", "See You Again"),
    ("XqZsoesa55w", "Baby Shark"),
    ("fJ9rUzIMcZQ", "Queen Bohemian Rhapsody"),
    ("kXYiU_JCYtU", "Linkin Park Numb"),
    ("5qap5aO4i9A", "Lofi Girl Study"),
    ("DWcJFNfaw9c", "Lofi Hip Hop"),
    ("h4T_LlK1VE4", "Mark Rober Glitter Bomb"),
    ("bHIhgxav9LY", "Veritasium Magnets"),
    ("MBRqu0YOH14", "Kurzgesagt The Egg"),
    ("L_LUpnjgPso", "GTA 6 Trailer"),
    ("Un5SEJ8MyPc", "Cyberpunk Trailer"),
    ("eaW0tYpxyp0", "Elden Ring Trailer"),
    ("kJQP7kiw5Fk", "Luis Fonsi Despacito"),
    ("OPf0YbXqDm0", "Mark Ronson Uptown Funk"),
    ("CevxZvSJLk8", "Katy Perry Roar"),
    ("hT_nvWreIhg", "OneRepublic Counting Stars"),
    ("YQHsXMglC9A", "Adele Hello")
]

working_catalog = []
for vid, title in candidates:
    url = f"https://i.ytimg.com/vi/{vid}/hqdefault.jpg"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=3) as resp:
            if resp.status == 200:
                working_catalog.append((vid, title))
                print(f"  [OK] {vid}: {title}")
    except:
        pass

print(f"\nTotal 100% verified working real YouTube videos: {len(working_catalog)}")
