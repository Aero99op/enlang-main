"""Friendly errors and persistence.

Builds enlng and enlngdb from source into a temp dir and checks:
  enlng    unknown power words give a clear SyntaxError (no raw gcc error), with a
           'show' suggestion for print-like words; valid programs are unaffected
  enlngdb  '--' comments are accepted; changes after 'use database X' are saved to
           X.edb (visible to a later run); a failed statement blocks the save and
           says so; a missing table gets a hint, including a typo suggestion
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXT = ".exe" if os.name == "nt" else ""
passed = failed = 0


def check(name, cond, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print("PASS ", name)
    else:
        failed += 1
        print("FAIL ", name, detail)


def build(tmp):
    enlng = os.path.join(tmp, "enlng" + EXT)
    enlngdb = os.path.join(tmp, "enlngdb" + EXT)
    subprocess.run(["gcc", "-O2", "-I.", "-Isrc", "enlng.c", "-lm", "-o", enlng], cwd=ROOT, check=True)
    subprocess.run(["gcc", "-O2", "-I.", "-Isrc", "-Ienlngdb/c", "enlngdb/c/main.c", "enlngdb/c/enlngdb.c",
                    "enlngdb/c/enlngdb_parser.c", "-lm", "-o", enlngdb], cwd=ROOT, check=True)
    return enlng, enlngdb


def write(path, text):
    with open(path, "w") as f:
        f.write(text)


def run(cmd, cwd, env=None):
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, env=env)
    return p.stdout, p.stderr


def main():
    tmp = tempfile.mkdtemp(prefix="enlangg-friendly-")
    try:
        enlng, enlngdb = build(tmp)
        work = os.path.join(tmp, "w")
        os.makedirs(work)

        # ---- enlng
        write(os.path.join(work, "a.enlng"), 'type enlng\n\nsay "hello"\n')
        out, err = run([enlng, "run", "a.enlng"], work)
        check("enlng: unknown word is a SyntaxError", "Unknown word 'say'" in err, err)
        check("enlng: suggests show for print-like words", "show" in err and "display" in err, err)
        check("enlng: no raw gcc error", "undeclared" not in err and "undeclared" not in out, err + out)

        write(os.path.join(work, "b.enlng"), "type enlng\n\nfoo 5\n")
        out, err = run([enlng, "run", "b.enlng"], work)
        check("enlng: other unknown word gets spelling hint", "Unknown word 'foo'" in err and "Check the spelling" in err, err)

        write(os.path.join(work, "c.enlng"),
              'type enlng\n\nremember total as 0\nfor i from 1 to 3 by 1:\n    total increases by i\ndisplay "sum" total\n')
        out, err = run([enlng, "run", "c.enlng"], work)
        check("enlng: valid program unaffected", out.strip() == "sum 6" and err.strip() == "", out + err)

        # ---- enlng interpreter: no compiler needed, runtime errors are readable
        no_cc = dict(os.environ, PATH=os.path.join(tmp, "empty-path"))
        out, err = run([enlng, "run", "c.enlng"], work, env=no_cc)
        check("enlng: runs with no C compiler on PATH", out.strip() == "sum 6" and err.strip() == "", out + err)

        write(os.path.join(work, "rt1.enlng"), "type enlng\n\nshow 1\nshow nope\n")
        out, err = run([enlng, "run", "rt1.enlng"], work, env=no_cc)
        check("enlng: undefined name is a RuntimeError with the line", "RuntimeError" in err and "rt1.enlng:4" in err and "I don't know a name 'nope'" in err, err)
        check("enlng: output before the error is kept", out.strip() == "1", out)

        write(os.path.join(work, "rt2.enlng"), "type enlng\n\naction f(x):\n    return x\n\nshow f(1, 2)\n")
        out, err = run([enlng, "run", "rt2.enlng"], work, env=no_cc)
        check("enlng: wrong number of inputs is explained", "'f' needs 1 input but was given 2" in err, err)

        write(os.path.join(work, "rt3.enlng"), "type enlng\n\naction loop(n):\n    return loop(n + 1)\n\nshow loop(1)\n")
        out, err = run([enlng, "run", "rt3.enlng"], work, env=no_cc)
        check("enlng: runaway recursion stops with a message", "called itself too many times" in err, err)

        # ---- enlngdb
        write(os.path.join(work, "add.enlngdb"),
              'type enlngdb\n\n-- add one student\nuse database school;\n'
              'insert into student with roll_no 106, name "Devansh", marks 88;\n')
        out, err = run([enlngdb, "add.enlngdb"], work)
        check("enlngdb: -- comment accepted", err.strip() == "" and "1 row affected" in out, out + err)
        check("enlngdb: saved to the active database file", os.path.exists(os.path.join(work, "school.edb")))
        check("enlngdb: not saved to the script-named file", not os.path.exists(os.path.join(work, "add.edb")))

        write(os.path.join(work, "read.enlngdb"), "type enlngdb\n\nuse database school;\nfind all records from student;\n")
        out, err = run([enlngdb, "read.enlngdb"], work)
        check("enlngdb: data survives to a later run", "Devansh" in out, out + err)

        write(os.path.join(work, "bad.enlngdb"),
              'type enlngdb\n\nuse database school;\ninsert into student with roll_no 107, name "Asha", marks 91;\n'
              "find all records from studnet;\n")
        out, err = run([enlngdb, "bad.enlngdb"], work)
        check("enlngdb: typo gets a did-you-mean", "Did you mean 'student'?" in out, out)
        check("enlngdb: lists existing tables", "Tables in 'school': student" in out, out)
        check("enlngdb: failed run says nothing was saved", "nothing from this run was saved" in out, out)
        out, err = run([enlngdb, "read.enlngdb"], work)
        check("enlngdb: failed run did not save its insert", "Asha" not in out, out)

        write(os.path.join(work, "new.enlngdb"), "type enlngdb\n\nuse database brandnew;\nfind all records from student;\n")
        out, err = run([enlngdb, "new.enlngdb"], work)
        check("enlngdb: empty database says how to create a table", "create table student with" in out, out)

        write(os.path.join(work, "u.enlngdb"), "type enlngdb\n\nuse database school;\nshow everything;\n")
        out, err = run([enlngdb, "u.enlngdb"], work)
        check("enlngdb: unknown statement mentions comments and commands", "Comments start with '#' or '--'" in err and "show tables" in err, err)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
