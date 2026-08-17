import subprocess

with open("youtube.html", "r", encoding="utf-8") as f:
    html = f.read()

if "<script>" in html:
    js = html.split("<script>")[1].split("</script>")[0]
    with open("scratch/check_syntax.js", "w", encoding="utf-8") as f:
        f.write(js)
    res = subprocess.run(["node", "--check", "scratch/check_syntax.js"], capture_output=True, text=True)
    print("STDOUT:", res.stdout)
    print("STDERR:", res.stderr)
    print("Return code:", res.returncode)
else:
    print("No script tag found!")
