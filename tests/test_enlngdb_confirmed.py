"""enlngdb: destructive statements need the word 'confirmed'.

Builds enlngdb from source into a temp dir, then checks:
  - refused statements change nothing, say REFUSED, name the fix, and print no stderr alert
  - confirmed statements run, print no refusal and no stderr output
  - filtered deletes need no confirmation
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = "enlngdb.exe" if os.name == "nt" else "enlngdb"
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
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    cmd = ["gcc", "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + ["-lm", "-o", exe]
    subprocess.run(cmd, cwd=ROOT, check=True)
    return exe


_runs = [0]


def run(exe, tmp, body):
    # a fresh database per run, since changes are saved between runs
    _runs[0] += 1
    script = os.path.join(tmp, "t.enlngdb")
    with open(script, "w") as f:
        f.write("type enlngdb\n\nuse database confirmtest%d;\n" % _runs[0] + body)
    p = subprocess.run([exe, script], cwd=tmp, capture_output=True, text=True)
    return p.stdout, p.stderr


SETUP = (
    'insert into student with roll_no 1, name "A", marks 80;\n'
    'insert into student with roll_no 2, name "B", marks 90;\n'
)


def main():
    tmp = tempfile.mkdtemp(prefix="enlngdb-confirm-")
    try:
        exe = build(tmp)

        out, err = run(exe, tmp, SETUP + "delete all from student;\nfind all records from student;\n")
        check("unconfirmed delete says REFUSED", "REFUSED" in out, out)
        check("unconfirmed delete names the keyword", "delete all from student confirmed;" in out, out)
        check("unconfirmed delete keeps rows", "2 row(s) in set" in out, out)
        check("unconfirmed delete is not reported as OK", "0 row(s) deleted" not in out, out)
        check("unconfirmed delete prints no stderr alert", "SECURITY ALERT" not in err, err)

        out, err = run(exe, tmp, SETUP + "drop table student;\nshow tables;\n")
        check("unconfirmed drop says REFUSED", "REFUSED" in out, out)
        check("unconfirmed drop names the keyword", "drop table student confirmed;" in out, out)
        check("unconfirmed drop keeps the table", "| student" in out, out)
        check("unconfirmed drop prints no stderr alert", "SECURITY ALERT" not in err, err)

        out, err = run(exe, tmp, SETUP + "delete all from student confirmed;\nfind all records from student;\n")
        check("confirmed delete removes rows", "2 row(s) deleted" in out and "0 row(s) in set" in out, out)
        check("confirmed delete has no refusal", "REFUSED" not in out, out)
        check("confirmed delete prints nothing on stderr", err.strip() == "", err)

        out, err = run(exe, tmp, SETUP + "drop table student CONFIRMED;\nshow tables;\n")
        check("confirmed drop works (CONFIRMED too)", "dropped successfully" in out and "0 table(s)" in out, out)
        check("confirmed drop prints nothing on stderr", err.strip() == "", err)

        out, err = run(exe, tmp, SETUP + "delete from student where roll_no is 1;\nfind all records from student;\n")
        check("filtered delete needs no confirmation", "1 row(s) deleted" in out and "REFUSED" not in out, out)

        out, err = run(exe, tmp, SETUP + "delete table student;\nshow tables;\n")
        check("unconfirmed delete table says REFUSED", "REFUSED" in out and "| student" in out, out)

        out, err = run(exe, tmp, SETUP + "delete table student confirmed;\nshow tables;\n")
        check("delete table confirmed drops it", "dropped successfully" in out and "0 table(s)" in out, out)

        out, err = run(exe, tmp, SETUP + "delete column marks from student;\nfind all records from student;\n")
        check("delete column removes it", "column 'marks' deleted" in out and "| marks" not in out and "| B" in out, out)

        out, err = run(exe, tmp, SETUP + "delete name from student;\nremove column marks in student;\nfind all records from student;\n")
        check("delete <col> and remove column work", "| name" not in out and "| marks" not in out and "| roll_no" in out, out)

        out, err = run(exe, tmp, SETUP + "delete column age from student;\n")
        check("unknown column is an error that lists the columns", "no column called 'age'" in out and "roll_no name marks" in out, out)

        out, err = run(exe, tmp, SETUP + "delete records from student where marks is 80;\nfind all records from student;\n")
        check("delete records ... where still deletes rows", "1 row(s) deleted" in out and "| marks" in out, out)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
