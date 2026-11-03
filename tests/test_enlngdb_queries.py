"""enlngdb queries: or, top/first/limit/offset, chosen columns, distinct,
sum/avg/min/max/count, group by, joins, add/drop column, insert by column name,
and the note when an insert replaces a row with the same id.

Builds enlngdb from source into a temp dir and runs small scripts with it.
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
        f.write("type enlngdb\n\nuse database querytest%d;\n" % _runs[0] + SETUP + body)
    p = subprocess.run([exe, script], cwd=tmp, capture_output=True, text=True)
    return p.stdout + p.stderr


def rows(out):
    """The data rows of every table printed, as lists of cells."""
    found = []
    for line in out.splitlines():
        if line.startswith("|"):
            found.append([c.strip() for c in line.strip("|").split("|")])
    return found


SETUP = (
    'create table student with id, name, marks, city\n'
    'insert into student with id 1, name "Asha", marks 80, city "Pune"\n'
    'insert into student with id 2, name "Ravi", marks 90, city "Goa"\n'
    'insert into student with id 3, name "Meera", marks 70, city "Pune"\n'
    'insert into student with city "Delhi", marks 95, name "Karan", id 4\n'
    'create table fees with fee_id, student_id, amount\n'
    'insert into fees with fee_id 10, student_id 1, amount 500\n'
    'insert into fees with fee_id 11, student_id 1, amount 300\n'
    'insert into fees with fee_id 12, student_id 2, amount 900\n'
)


def main():
    tmp = tempfile.mkdtemp(prefix="enlngdb-query-")
    try:
        exe = build(tmp)

        out = run(exe, tmp, "find records from student where id is 4\n")
        check("insert puts values by column name", ["4", "Karan", "95", "Delhi"] in rows(out), out)

        out = run(exe, tmp, 'find name from student where city is "Goa" or marks is less than 75\n')
        check("or", rows(out) == [["name"], ["Ravi"], ["Meera"]], out)

        out = run(exe, tmp, 'find name from student where (city is "Pune" or city is "Goa") and marks is at least 80\n')
        check("and before or, with ( )", rows(out) == [["name"], ["Asha"], ["Ravi"]], out)

        out = run(exe, tmp, "find first 2 records from student\n")
        check("first N", len(rows(out)) == 3, out)

        out = run(exe, tmp, "find top 1 records from student sorted by marks highest first\n")
        check("top N with highest first", rows(out)[1][1] == "Karan" and len(rows(out)) == 2, out)

        out = run(exe, tmp, "find name from student order by id asc limit 2 offset 1\n")
        check("limit and offset", rows(out) == [["name"], ["Ravi"], ["Meera"]], out)

        out = run(exe, tmp, "find name, marks from student where id is 1\n")
        check("chosen columns", rows(out) == [["name", "marks"], ["Asha", "80"]], out)

        out = run(exe, tmp, "select distinct city from student\n")
        check("distinct", rows(out) == [["city"], ["Pune"], ["Goa"], ["Delhi"]], out)

        out = run(exe, tmp, "select sum(marks), avg(marks), min(marks), max(marks), count(*) from student\n")
        check("aggregates", rows(out)[1] == ["335", "83.75", "70", "95", "4"], out)

        out = run(exe, tmp, 'select sum(marks) from student where city is "Nowhere"\n')
        check("sum of no rows is null", rows(out)[1] == ["null"], out)

        out = run(exe, tmp, "find city, count(*), avg(marks) from student group by city order by count(*) desc\n")
        check("group by", rows(out)[1] == ["Pune", "2", "75.00"] and len(rows(out)) == 4, out)

        out = run(exe, tmp, "count records in student group by city\n")
        check("count ... group by", rows(out)[0] == ["city", "count(*)"] and len(rows(out)) == 4, out)

        out = run(exe, tmp, 'count records in student where marks is above 75 and city is "Pune"\n')
        check("count with and", rows(out) == [["count(*)"], ["1"]], out)

        out = run(exe, tmp, 'find student where name like "M%"\n')
        check("find <table> where, without from", rows(out)[1][1] == "Meera" and len(rows(out)) == 2, out)

        out = run(exe, tmp, "find all the records from student where id is 2\n")
        check("'all the records' still means every column", rows(out)[0] == ["id", "name", "marks", "city"], out)

        out = run(exe, tmp, "find student.name, fees.amount from student inner join fees on student.id is fees.student_id where fees.amount is greater than 400\n")
        check("inner join", rows(out) == [["student.name", "fees.amount"], ["Asha", "500"], ["Ravi", "900"]], out)

        out = run(exe, tmp, "find name, amount from student left join fees on id = student_id\n")
        check("left join keeps rows with no match", ["Meera", "null"] in rows(out) and len(rows(out)) == 6, out)

        out = run(exe, tmp, "find name, sum(amount) from student join fees on id is student_id group by name\n")
        check("join with group by", rows(out) == [["name", "sum(amount)"], ["Asha", "800"], ["Ravi", "900"]], out)

        out = run(exe, tmp, 'insert into student with id 2, name "Ravi K", marks 91, city "Goa"\nfind name from student where id is 2\n')
        check("same id says the row was replaced", "1 row replaced" in out and "id 2" in out, out)
        check("same id keeps one row", rows(out) == [["name"], ["Ravi K"]], out)

        out = run(exe, tmp, "alter table student add column age as integer default 18\nadd column phone to student\nfind id, age, phone from student where id is 1\n")
        check("add column with a default", rows(out) == [["id", "age", "phone"], ["1", "18", "null"]], out)

        out = run(exe, tmp, "alter table student drop column marks confirmed\nfind records from student where id is 1\n")
        check("alter table drop column", rows(out)[0] == ["id", "name", "city"], out)

        out = run(exe, tmp, "find nme from student\n")
        check("unknown column lists the columns", "no column 'nme'" in out and "id name marks city" in out, out)

        out = run(exe, tmp, "find name from student join fees on id is student_id where fee_id is 10 order\n")
        check("a word it does not know is an error", "don't understand 'order'" in out, out)

        out = run(exe, tmp, 'find records from student where name is 5\n')
        check("a number is not equal to a name", len(rows(out)) == 1, out)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
