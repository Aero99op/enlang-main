import sys
def search_in_file(filename, query):
    with open(filename, "r", encoding="utf-8", errors="ignore") as f:
        for i, line in enumerate(f):
            if query in line:
                print(f"{filename}:{i+1}: {line.strip()}")

search_in_file("portfolio.enlgf", "padding")
search_in_file("portfolio.html", "padding")
search_in_file("portfolio.enlgd", "padding")
