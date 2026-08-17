import subprocess

res = subprocess.run(["node", "--check", "scratch/check_syntax.js"], capture_output=True)
print("Return code:", res.returncode)
print("STDERR:\n", res.stderr.decode("utf-8", errors="replace"))
