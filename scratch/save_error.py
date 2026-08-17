import subprocess

res = subprocess.run(["node", "--check", "scratch/check_syntax.js"], capture_output=True)
with open("scratch/error.txt", "w", encoding="utf-8") as f:
    f.write(f"Return code: {res.returncode}\n")
    f.write("STDERR:\n")
    f.write(res.stderr.decode("utf-8", errors="replace"))
print("Wrote error to scratch/error.txt")
