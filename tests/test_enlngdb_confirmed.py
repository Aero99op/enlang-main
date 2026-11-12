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


# ENLNGDB_TEST_CC names another compiler (clang for a 64-bit build); the C library of Visual C++ has no libm
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]

def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    cmd = [CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe]
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

        out, err = run(exe, tmp, SETUP + "delete column marks from student;\n")
        check("delete column without 'confirmed' is refused: the column goes from every row", "REFUSED" in out and "delete column marks from student confirmed" not in out and "drop column marks confirmed" in out, out)
        out, err = run(exe, tmp, SETUP + "delete column marks from student confirmed;\nfind all records from student;\n")
        check("delete column removes it", "column 'marks' deleted" in out and "| marks" not in out and "| B" in out, out)

        out, err = run(exe, tmp, SETUP + "delete name from student confirmed;\nremove column marks in student confirmed;\nfind all records from student;\n")
        check("delete <col> and remove column work", "| name" not in out and "| marks" not in out and "| roll_no" in out, out)

        out, err = run(exe, tmp, SETUP + "delete column age from student;\n")
        check("unknown column is an error that lists the columns", "no column called 'age'" in out and "roll_no name marks" in out, out)

        out, err = run(exe, tmp, SETUP + "delete records from student where marks is 80;\nfind all records from student;\n")
        check("delete records ... where still deletes rows", "1 row(s) deleted" in out and "| marks" in out, out)

        # ---- a change of every row needs 'confirmed' ----
        out, err = run(exe, tmp, SETUP + "update student set marks = 1;\nfind marks from student;\n")
        check("update with no 'where' is refused, and nothing changes", "REFUSED" in out and "update student set ... confirmed" in out and "| 80" in out, out)
        out, err = run(exe, tmp, SETUP + "update student set marks = 1 confirmed;\nfind marks from student;\n")
        check("... with 'confirmed' it changes every row", "2 row(s) updated" in out and out.count("| 1 ") == 2, out)
        out, err = run(exe, tmp, SETUP + "update student set marks = 5 where marks > 0;\n")
        check("update whose 'where' every row meets is refused too", "REFUSED" in out and "every one of the 2 rows" in out, out)
        out, err = run(exe, tmp, SETUP + "update student set marks = 5 where roll_no is 1;\nupdate student set marks = 6 limit 1;\nfind marks from student;\n")
        check("update of some rows, or with a limit, needs no 'confirmed'", "REFUSED" not in out and "2 row(s) updated" not in out and "| 6 " in out, out)
        out, err = run(exe, tmp, SETUP + "delete from student where marks > 0;\ncount records in student;\n")
        check("delete whose 'where' every row meets is refused", "REFUSED" in out and "every one of the 2 rows" in out, out)
        out, err = run(exe, tmp, SETUP + "delete from student where marks > 0 confirmed;\ncount records in student;\n")
        check("... with 'confirmed' it deletes them", "2 row(s) deleted" in out, out)
        out, err = run(exe, tmp, SETUP + "preview update student set marks = 0;\npreview delete from student where marks > 0;\n")
        check("preview shows a change of every row without asking, and changes nothing", "REFUSED" not in out and out.count("PREVIEW only") == 2, out)
        out, err = run(exe, tmp, SETUP + "alter table student drop column marks;\n")
        check("alter table ... drop column is refused without 'confirmed'", "REFUSED" in out and "drop column marks confirmed" in out, out)
        out, err = run(exe, tmp, SETUP + "alter table student drop column marks confirmed;\ndescribe student;\n")
        check("... and done with it", "| marks" not in out and "REFUSED" not in out, out)
        with open(os.path.join(tmp, "more.csv"), "w") as f:
            f.write("id,name,marks\n2,Bee,95\n3,C,70\n")
        KEYED = "create table student with id integer primary key, name text, marks integer;\ninsert into student values (1, \"A\", 80), (2, \"B\", 90);\n"
        out, err = run(exe, tmp, KEYED + 'import "more.csv" into student;\ncount records in student;\n')
        check("import that would write over a row already there is refused, and imports nothing", "REFUSED" in out and "1 row(s) of 'more.csv'" in out and "| 2 " in out, out)
        out, err = run(exe, tmp, KEYED + 'import "more.csv" into student confirmed;\nfind name from student where id is 2;\n')
        check("... with 'confirmed' it imports, and replaces that row", "1 of them replaced" in out and "Bee" in out, out)
        out, err = run(exe, tmp, 'insert into one with id 1, v 1;\nupdate one set v = 2 where v is 1;\nfind v from one;\n')
        check("a table of one row is not asked about", "REFUSED" not in out and "| 2 " in out, out)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
