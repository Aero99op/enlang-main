"""Checks the tools around the language: the debugger and packages.

    python tests/test_tools.py <path-to-enlng>

Each check runs `enlng` in a fresh folder, so nothing in the repo changes.
"""
import os
import shutil
import subprocess
import sys
import tempfile

PROGRAM = """type enlng

function area with width, height:
    remember result as width times height
    give result

remember total as 0
for each size in [2, 3]:
    total = total plus area(size, 4)
show total
"""

failed = []


def check(name, ok, detail=""):
    print(("ok    " if ok else "FAIL  ") + name + ("" if ok else "\n      " + str(detail)))
    if not ok:
        failed.append(name)


def run(exe, args, cwd, stdin=""):
    p = subprocess.run([exe] + args, cwd=cwd, input=stdin.encode(), capture_output=True, timeout=60)
    return p.returncode, p.stdout.decode("utf-8", "replace").replace("\r\n", "\n"), p.stderr.decode("utf-8", "replace")


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def test_debugger(exe, work):
    write(os.path.join(work, "prog.enlng"), PROGRAM)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "step\nstep\nvars\nq\n")
    check("debug stops at the first statement", "->   7 | remember total as 0" in out, out)
    check("debug shows the names here", "total = 0  (a number)" in out, out)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "next\nnext\nnext\nstep\nwhere\np width plus 1\nq\n")
    check("step goes into a call", "-> " in out and "remember result as width times height" in out, out)
    check("where shows the calls", "area (called from line 9)" in out, out)
    check("print works out an expression", "width plus 1 = 4  (a number)" in out, out)

    rc, out, _ = run(exe, ["debug", "prog.enlng", "--break", "10"], work, "c\np total\nc\n")
    check("a breakpoint given on the command line", "->  10 | show total" in out and "total = 20  (a number)" in out, out)
    check("the program finishes after continue", out.rstrip().endswith("20"), out)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "break area\nc\nbreaks\nq\n")
    check("break on a function name", "Will stop at line 3." in out, out)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "p zzz\np 1 plus\nq\n")
    check("a mistake in print does not stop the debugger", out.count("Problem") + out.count("not a value") >= 2 and "(debug)" in out, out)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "out\nq\n")
    check("out outside a call just goes on", "(debug)" in out, out)

    rc, out, _ = run(exe, ["debug", "prog.enlng"], work, "")
    check("with no more commands the program runs to the end", out.rstrip().endswith("20") and rc == 0, (rc, out))

    # a function from another file is run through without stopping in it
    write(os.path.join(work, "lib.enlng"), "type enlng\n\nfunction double with n:\n    give n times 2\n")
    write(os.path.join(work, "uses.enlng"), 'type enlng\nuse "lib.enlng"\nremember x as double(4)\nshow x\n')
    rc, out, _ = run(exe, ["debug", "uses.enlng"], work, "step\nstep\nstep\nq\n")
    check("lines of a used file are not shown", "give n times 2" not in out, out)


def test_standard_library(exe, work):
    # lib_strings.enlng is not in this folder: it comes with the language
    write(os.path.join(work, "std.enlng"), 'type enlng\nuse "lib_strings.enlng"\nshow 1\n')
    rc, out, err = run(exe, ["run", "std.enlng"], work)
    check("use finds the standard library files", rc == 0 and out == "1\n", (rc, out, err))


def test_packages(exe, work):
    pkg = os.path.join(work, "src", "geometry")
    write(os.path.join(pkg, "main.enlng"), 'type enlng\nuse "helpers.enlng"\nfunction circle_area with r:\n    give pi_value() times r times r\n')
    write(os.path.join(pkg, "helpers.enlng"), "type enlng\nfunction pi_value:\n    give 3\n")
    write(os.path.join(work, "src", "greeter.enlng"), 'type enlng\nfunction hello with name:\n    give "Hello, " + name\n')
    app = os.path.join(work, "app")
    write(os.path.join(app, "main.enlng"),
          'type enlng\nuse "geometry"\nuse "greeter"\nshow circle_area(2)\nshow hello("Asha")\n')
    write(os.path.join(app, "deep", "er", "again.enlng"), 'type enlng\nuse "greeter"\nshow hello("Rao")\n')

    rc, out, _ = run(exe, ["run", "main.enlng"], app)
    check("an uninstalled package is reported", rc != 0 and "There is no package called" in out + _, (rc, out, _))

    rc, out, err = run(exe, ["add", "../src/geometry"], app)
    check("add a folder", rc == 0 and "Installed 'geometry'" in out, (rc, out, err))
    rc, out, err = run(exe, ["add", "../src/greeter.enlng"], app)
    check("add a single file", rc == 0 and "Installed 'greeter'" in out, (rc, out, err))
    rc, out, err = run(exe, ["run", "main.enlng"], app)
    check("use a package by name", out == "12\nHello, Asha\n", (rc, out, err))
    rc, out, err = run(exe, ["run", "--native", "main.enlng"], app)
    check("the same with --native", out == "12\nHello, Asha\n", (rc, out, err))
    rc, out, err = run(exe, ["run", os.path.join("deep", "er", "again.enlng")], app)
    check("found from a folder below", out == "Hello, Rao\n", (rc, out, err))

    rc, out, err = run(exe, ["packages"], app)
    check("list the packages", "geometry" in out and "greeter" in out, out)
    rc, out, err = run(exe, ["remove", "greeter"], app)
    check("remove a package", rc == 0 and not os.path.exists(os.path.join(app, "enlng_packages", "greeter.enlng")), (rc, out, err))
    shutil.rmtree(os.path.join(app, "enlng_packages"))
    rc, out, err = run(exe, ["install"], app)
    check("install brings back what the list names", rc == 0 and os.path.exists(os.path.join(app, "enlng_packages", "geometry", "main.enlng")), (rc, out, err))

    for bad in (["add", "no_such_thing"], ["add", "a&b"], ["add", "../src/geometry", "../evil"], ["remove", "../x"]):
        rc, out, err = run(exe, bad, app)
        check("refused: " + " ".join(bad), rc != 0, (rc, out, err))

    write(os.path.join(work, "src", "empty", "readme.txt"), "nothing to use here")
    rc, out, err = run(exe, ["add", "../src/empty"], app)
    check("a folder without main.enlng is refused", rc != 0 and not os.path.exists(os.path.join(app, "enlng_packages", "empty")), (rc, out, err))

    # a package from git
    if shutil.which("git"):
        repo = os.path.join(work, "gitpkg")
        write(os.path.join(repo, "main.enlng"), "type enlng\nfunction twice with x:\n    give x times 2\n")
        git = ["git", "-c", "user.email=t@t", "-c", "user.name=t"]
        subprocess.run(git + ["init", "-q"], cwd=repo, capture_output=True)
        subprocess.run(git + ["add", "."], cwd=repo, capture_output=True)
        subprocess.run(git + ["commit", "-q", "-m", "first"], cwd=repo, capture_output=True)
        write(os.path.join(app, "g.enlng"), 'type enlng\nuse "gitdemo"\nshow twice(21)\n')
        url = "file:///" + repo.replace("\\", "/")
        rc, out, err = run(exe, ["add", url, "gitdemo"], app)
        check("add from a git address", rc == 0, (rc, out, err))
        rc, out, err = run(exe, ["run", "g.enlng"], app)
        check("use the package from git", out == "42\n", (rc, out, err))
        check("the git folder is not kept", not os.path.exists(os.path.join(app, "enlng_packages", "gitdemo", ".git")))
    else:
        print("skip  git is not installed")


def main():
    exe = os.path.abspath(sys.argv[1])
    for fn in (test_debugger, test_standard_library, test_packages):
        work = tempfile.mkdtemp(prefix="enlng-tools-")
        try:
            fn(exe, work)
        finally:
            shutil.rmtree(work, ignore_errors=True)
    print("\n%d failed" % len(failed) if failed else "\nall passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
