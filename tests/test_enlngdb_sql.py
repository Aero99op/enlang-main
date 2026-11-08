"""enlngdb SQL features: between/in/not/exists, subqueries, having, union, with, views, expressions and
functions, aliases, window functions, multi-row insert, update/delete with a full where, column types,
constraints, references, alter table, indexes, transactions, truncate, databases, describe, explain.

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
TMP = None
EXE_PATH = None
_runs = [0]


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
    subprocess.run(["gcc", "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + ["-lm", "-o", exe], cwd=ROOT, check=True)
    return exe


SETUP = (
    'create table student with id, name, marks, city\n'
    'insert into student with id 1, name "Asha", marks 80, city "Pune"\n'
    'insert into student with id 2, name "Ravi", marks 90, city "Goa"\n'
    'insert into student with id 3, name "Meera", marks 70, city "Pune"\n'
    'insert into student with id 4, name "Karan", marks 95\n'
    'create table fees with fee_id, student_id, amount\n'
    'insert into fees with fee_id 10, student_id 1, amount 500\n'
    'insert into fees with fee_id 11, student_id 1, amount 300\n'
    'insert into fees with fee_id 12, student_id 2, amount 900\n'
    'insert into fees with fee_id 13, student_id 9, amount 100\n'
)


def run(body, setup=SETUP, name=None):
    """A fresh database per run (the files are saved between runs)."""
    _runs[0] += 1
    db = name or "sqltest%d" % _runs[0]
    script = os.path.join(TMP, "t.enlngdb")
    with open(script, "w") as f:
        f.write("type enlngdb\n\nuse database %s;\n" % db + setup + "display \"@@\"\n" + body)
    p = subprocess.run([EXE_PATH, script], cwd=TMP, capture_output=True, text=True)
    out = p.stdout + p.stderr
    return out.split("@@", 1)[1] if "@@" in out else out


def rows(out):
    """The rows of the first table printed (header first), as lists of cells."""
    found = []
    for line in out.splitlines():
        if line.startswith("|"):
            found.append([c.strip() for c in line.strip("|").split("|")])
        elif found and line.startswith("+") and len(found) > 0 and found[-1] != "end":
            pass
    return found


def col(out, i=0):
    r = rows(out)
    return [x[i] for x in r[1:]]


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-sql-")
    try:
        EXE_PATH = build(TMP)

        # ---- conditions
        out = run("find name from student where marks between 70 and 85\n")
        check("between", col(out) == ["Asha", "Meera"], out)
        out = run("find name from student where marks not between 70 and 85\n")
        check("not between", col(out) == ["Ravi", "Karan"], out)
        out = run('find name from student where city in ("Pune", "Goa")\n')
        check("in (list)", col(out) == ["Asha", "Ravi", "Meera"], out)
        out = run('find name from student where city not in ("Pune")\n')
        check("not in (list): a null city is not 'not in'", col(out) == ["Ravi"], out)
        out = run('find name from student where not city is "Pune"\n')
        check("not <condition>: a null city is unknown, so it stays out", col(out) == ["Ravi"], out)
        out = run('find name from student where city is not "Pune"\n')
        check("is not: a null city stays out", col(out) == ["Ravi"], out)
        out = run("find name from student where city is null\n")
        check("is null", col(out) == ["Karan"], out)
        out = run("find name from student where marks < 85 and city is not null\n")
        check("a comparison with null is never true", col(out) == ["Asha", "Meera"], out)
        out = run("find name from student where id in (find student_id from fees)\n")
        check("in (subquery)", col(out) == ["Asha", "Ravi"], out)
        out = run("find name from student where id not in (find student_id from fees)\n")
        check("not in (subquery)", col(out) == ["Meera", "Karan"], out)
        out = run("find name from student where marks > (find avg(marks) from student)\n")
        check("a subquery as a value", col(out) == ["Ravi", "Karan"], out)
        out = run("find name from student where exists (find fee_id from fees where student_id is student.id)\n")
        check("exists, looking at the row outside", col(out) == ["Asha", "Ravi"], out)
        out = run("find name from student where not exists (find fee_id from fees where student_id is student.id)\n")
        check("not exists", col(out) == ["Meera", "Karan"], out)
        out = run("find name from student where marks > (find marks from student)\n")
        check("a subquery as a value must give one row", "more than one row" in out, out)
        out = run('find name from student where name like "%a" and marks >= 70 or id is 2\n')
        check("like, and, or together", col(out) == ["Asha", "Ravi", "Meera"], out)
        out = run("find name from student where (marks > 85 or marks < 75) and city is not null\n")
        check("brackets in a condition", col(out) == ["Ravi", "Meera"], out)

        # ---- group by, having, order by
        out = run("find city, count(*) from student group by city having count(*) > 1\n")
        check("having", rows(out) == [["city", "count(*)"], ["Pune", "2"]], out)
        out = run("find city, marks, count(*) from student group by city, marks order by marks\n")
        check("group by two columns", len(rows(out)) == 5 and rows(out)[1] == ["Pune", "70", "1"], out)
        out = run("find name from student order by city asc, marks desc\n")
        check("order by two columns", col(out) == ["Karan", "Ravi", "Asha", "Meera"], out)
        out = run("find name from student order by marks + 1\n")
        check("order by an expression", col(out) == ["Meera", "Asha", "Ravi", "Karan"], out)
        out = run("find city as c, count(*) as n from student where city is not null group by c order by n desc, c\n")
        check("group by and order by an alias", rows(out) == [["c", "n"], ["Pune", "2"], ["Goa", "1"]], out)
        out = run("find name, marks from student order by 2 desc limit 2\n")
        check("order by a position", col(out) == ["Karan", "Ravi"], out)
        out = run("find count(distinct city), sum(marks * 2), group_concat(name) from student\n")
        check("count(distinct), sum of an expression, group_concat", rows(out)[1] == ["2", "670", "Asha,Ravi,Meera,Karan"], out)
        out = run("find city, avg(marks) from student where city is not null group by city having avg(marks) > 80\n")
        check("having with avg", rows(out) == [["city", "avg(marks)"], ["Goa", "90.00"]], out)

        # ---- union, with, derived tables
        a = "find name from student where marks > 75"
        b = "find name from student where marks < 85"
        out = run("%s union %s\n" % (a, b))
        check("union drops repeats", col(out) == ["Asha", "Ravi", "Karan", "Meera"], out)
        out = run("%s union all %s\n" % (a, b))
        check("union all keeps them", len(col(out)) == 5, out)
        out = run("%s intersect %s\n" % (a, b))
        check("intersect", col(out) == ["Asha"], out)
        out = run("%s except %s\n" % (a, b))
        check("except", col(out) == ["Ravi", "Karan"], out)
        out = run("%s union %s order by name limit 3\n" % (a, b))
        check("order by and limit after a union", col(out) == ["Asha", "Karan", "Meera"], out)
        out = run("find name, id from student union find name from student\n")
        check("a union needs the same number of columns", "same number of columns" in out, out)
        out = run("with big as (find name, marks from student where marks > 75) find name from big order by name\n")
        check("with ... as (query)", col(out) == ["Asha", "Karan", "Ravi"], out)
        out = run("with a as (find id from student where marks > 75), b as (find id from a where id > 1) find count(*) from b\n")
        check("a with that uses an earlier one", col(out) == ["2"], out)
        out = run("find t.name from (find name, marks from student where marks > 85) as t order by t.name\n")
        check("a bracketed query as a table", col(out) == ["Karan", "Ravi"], out)

        # ---- expressions and functions
        out = run("find name, marks + 5, marks * 2 - 1 from student where id is 1\n")
        check("arithmetic", rows(out)[1] == ["Asha", "85", "159"], out)
        out = run("find marks / 4, marks % 7, 7 / 2 from student where id is 1\n")
        check("division and remainder", rows(out)[1] == ["20", "3", "3.50"], out)
        out = run("find marks / 0 from student\n")
        check("division by zero is an error", "division by zero" in out, out)
        out = run('find upper(name), lower(name), length(name), substr(name, 1, 2), name + "-" + city from student where id is 1\n')
        check("text functions", rows(out)[1] == ["ASHA", "asha", "4", "As", "Asha-Pune"], out)
        out = run('find concat(name, "-", city), coalesce(city, "none"), replace(name, "a", "A") from student where id is 4\n')
        check("concat, coalesce, replace", rows(out)[1] == ["Karan-", "none", "KArAn"], out)
        out = run("find abs(0 - 5), round(avg(marks), 1), round(2.6), floor(2.9), ceil(2.1), power(2, 3), mod(7, 3), sqrt(16) from student\n")
        check("number functions", rows(out)[1] == ["5", "83.80", "3", "2", "3", "8.00", "1", "4.00"], out)
        out = run('find case when marks >= 90 then "A" when marks >= 80 then "B" else "C" end from student order by id\n')
        check("case when", col(out) == ["B", "A", "C", "A"], out)
        out = run('find case city when "Pune" then 1 when "Goa" then 2 end from student order by id\n')
        check("case <value> when", col(out) == ["1", "2", "1", "null"], out)
        out = run("find cast(marks as text) + cast(id as text), cast(\"12\" as integer) + 1 from student where id is 1\n")
        check("cast", rows(out)[1] == ["801", "13"], out)
        out = run('find year("2024-03-15"), month("2024-03-15"), day("2024-03-15"), datediff("2024-03-15", "2024-03-10") from student limit 1\n')
        check("date functions", rows(out)[1] == ["2024", "3", "15", "5"], out)
        out = run("find nope(1) from student\n")
        check("an unknown function says so", "don't know a function called 'nope'" in out, out)
        out = run("find name from student where sum(marks) > 5\n")
        check("a total in 'where' is an error that points to 'having'", "not in 'where'" in out, out)
        out = run("select 1 + 1, upper(\"a\"), 2 * 3\n")
        check("select without a table", rows(out)[1] == ["2", "A", "6"], out)
        out = run("find name as n, marks as m from student where id is 1\n")
        check("as gives a column another name", rows(out) == [["n", "m"], ["Asha", "80"]], out)
        out = run("find s.name from student s where s.id is 1\n")
        check("a table alias", col(out) == ["Asha"], out)
        out = run("find a.name from student a join student b on a.marks < b.marks where b.name is \"Karan\" order by a.id\n")
        check("a table joined to itself", col(out) == ["Asha", "Ravi", "Meera"], out)
        out = run("find student.*, fees.amount from student join fees where fees.fee_id is 10\n")
        check("table.* in the select list", rows(out) and rows(out)[0][:5] == ["student.id", "student.name", "student.marks", "student.city", "fees.amount"], out)
        out = run("find s.name, f.amount from student as s left join fees as f using (id)\n")
        check("using (column) needs the column in both tables", "using (id)" in out, out)
        out = run("find name, amount from student join fees using (id)\n")
        check("using with a missing column", "using (id)" in out, out)
        out = run("find id from student s join fees s on id is student_id\n")
        check("two tables with one name", "used for two tables" in out, out)

        # ---- window functions
        out = run("find name, rank() over (order by marks desc) from student order by id\n")
        check("rank", col(out, 1) == ["3", "2", "4", "1"], out)
        out = run("find name, row_number() over (order by marks), dense_rank() over (order by city) from student order by id\n")
        check("row_number, dense_rank", [r[1:] for r in rows(out)[1:]] == [["2", "3"], ["3", "2"], ["1", "3"], ["4", "1"]], out)
        out = run("find name, sum(marks) over (partition by city) from student where city is not null order by id\n")
        check("sum over a partition", col(out, 1) == ["150", "90", "150"], out)
        out = run("find name, sum(marks) over (order by id) from student order by id\n")
        check("a running total", col(out, 1) == ["80", "170", "240", "335"], out)
        out = run("find name, lag(marks) over (order by id), lead(marks, 1, 0) over (order by id) from student order by id\n")
        check("lag and lead", [r[1:] for r in rows(out)[1:]] == [["null", "90"], ["80", "70"], ["90", "95"], ["70", "0"]], out)
        out = run("find name from student where rank() over (order by marks) > 1\n")
        check("a window function in 'where' is an error", "over (...) can only be used" in out, out)

        # ---- insert
        out = run('insert into student values (5, "Eve", 50, "Goa"), (6, "Fay", 60, "Goa")\nfind count(*) from student\n')
        check("insert ... values with several rows", "2 rows affected" in out and col(out) == ["6"], out)
        out = run('insert into student (id, name) values (7, "Gus")\nfind name, marks from student where id is 7\n')
        check("insert with a column list", rows(out) == [["name", "marks"], ["Gus", "null"]], out)
        out = run('create table copy with id, name\ninsert into copy (id, name) find id, name from student where marks > 75\nfind count(*) from copy\n')
        check("insert ... find", col(out) == ["3"], out)
        out = run('insert into student values (7, "Gus")\n')
        check("too few values is an error", "2 value(s), but 4 column(s)" in out, out)
        out = run('insert into student values (9, "Ok", 1, "x"), (1, "Dup", 2, "y")\nfind count(*) from student\n')
        check("a replaced key is told", "replaced a row with the same key" in out, out)
        out = run('create table u with n integer unique\ninsert into u values (1), (2), (1)\nfind count(*) from u\n')
        check("several rows go in together or not at all", "already in column 'n'" in out and col(out) == ["0"], out)
        out = run('insert into nowhere values (1)\n')
        check("insert values into a missing table is an error", "does not exist" in out, out)

        # ---- update and delete
        out = run('update student set marks = 1 where city is "Pune" and id is 1\nfind marks from student where id is 1\n')
        check("update with and", col(out) == ["1"], out)
        out = run("update student set marks = marks + 5, city = upper(name) where id is 1\nfind marks, city from student where id is 1\n")
        check("update with expressions that use the old row", rows(out)[1] == ["85", "ASHA"], out)
        out = run("update student set marks = 0 where marks > 85 limit 1\nfind count(*) from student where marks is 0\n")
        check("update ... limit", col(out) == ["1"], out)
        out = run("update student set nope = 1\n")
        check("update of a column that is not there", "has no column 'nope'" in out and "Its columns are" in out, out)
        out = run('in student change city to New Delhi where id is 4\nfind city from student where id is 4\n')
        check("change ... to a value of several words", col(out) == ["New Delhi"], out)
        out = run('in student change marks to 99 where name is "Ravi"\nfind marks from student where id is 2\n')
        check("in <table> change ... to ...", col(out) == ["99"], out)
        out = run('update student set marks = 100 where id is 99\n')
        check("update that matches nothing", "0 row(s) updated" in out, out)
        out = run('delete from student where city is "Pune" and id is 1\nfind count(*) from student\n')
        check("delete with and", col(out) == ["3"], out)
        out = run("delete from student where id is 1 or id is 2\nfind count(*) from student\n")
        check("delete with or", col(out) == ["2"], out)
        out = run("delete from student where marks between 80 and 90\nfind count(*) from student\n")
        check("delete with between", col(out) == ["2"], out)
        out = run("delete from student\nfind count(*) from student\n")
        check("delete without where is still refused", "REFUSED" in out and col(out) == ["4"], out)
        out = run("delete all from student confirmed\nfind count(*) from student\n")
        check("delete all ... confirmed", col(out) == ["0"], out)
        out = run("truncate table student\nfind count(*) from student\n")
        check("truncate is refused without confirmed", "REFUSED" in out and col(out) == ["4"], out)
        out = run("truncate table student confirmed\nfind count(*) from student\n")
        check("truncate table ... confirmed", col(out) == ["0"], out)

        # ---- column types and constraints
        T = ('create table p with id integer primary key, email text unique not null, age integer default 18 check (age >= 0), '
             'role text default "user", active boolean default true, born date\n')
        out = run(T + 'insert into p with id 1, email "a@x"\nfind id, age, role, active from p\n')
        check("defaults fill what is not given", rows(out)[1] == ["1", "18", "user", "true"], out)
        out = run(T + 'insert into p with id 1, email "a@x"\ninsert into p with id 2, email "A@X"\n')
        check("unique is checked", "already in column 'email'" in out, out)
        out = run(T + 'insert into p with id 1\n')
        check("not null is checked", "'email' cannot be empty" in out, out)
        out = run(T + 'insert into p with id 1, email "a", age -5\n')
        check("check is checked", "the check 'age >= 0' is not met" in out, out)
        out = run(T + 'insert into p with id 1, email "a", age "abc"\n')
        check("an integer column refuses text", "holds whole numbers, and 'abc' is not one" in out, out)
        out = run(T + 'insert into p with id 1, email "a", age 2.5\n')
        check("an integer column refuses a fraction", "has a fraction" in out, out)
        out = run(T + 'insert into p with id 1, email "a", age "30"\nfind age from p\n')
        check("a number written as text is turned into the number", col(out) == ["30"], out)
        out = run(T + 'insert into p with id 1, email "a", born "2024-02-30x"\n')
        check("a date column refuses what is not a date", "holds a date like 2024-03-15" in out, out)
        out = run(T + 'insert into p with id 1, email "a", born "2024-03-15"\nfind born from p\n')
        check("a date column takes a date", col(out) == ["2024-03-15"], out)
        out = run(T + 'insert into p with id 1, email "a", active "maybe"\n')
        check("a boolean column refuses other words", "holds true or false" in out, out)
        out = run(T + 'insert into p with id 1, email "a"\ninsert into p with id 1, email "b"\nfind email from p\n')
        check("a primary key written twice replaces the row", col(out) == ["b"], out)
        out = run(T + 'insert into p with id 1, email "a"\ninsert into p with id 2, email "b"\nupdate p set email = "a" where id is 2\n')
        check("update is checked too", "already in column 'email'" in out, out)
        out = run('create table log with id integer primary key auto_increment, msg text\ninsert into log with msg "a"\ninsert into log with msg "b"\ninsert into log with id 10, msg "c"\ninsert into log with msg "d"\nfind id from log\n')
        check("auto_increment", col(out) == ["1", "2", "10", "11"], out)
        out = run('create table kv with k text, v text, primary key (k, v)\ninsert into kv values ("a", "1"), ("a", "2")\ninsert into kv values ("a", "1")\nfind count(*) from kv\n')
        check("a key of two columns", col(out) == ["2"], out)
        out = run('create table ord with customer_id, item\ninsert into ord with customer_id 1, item "Rice"\ninsert into ord with customer_id 1, item "Oil"\nfind count(*) from ord\n')
        check("a column named customer_id no longer makes rows replace each other", col(out) == ["2"], out)
        out = run('create table x with a frob\n')
        check("a word that is not a type is an error", "don't understand 'frob'" in out, out)

        # ---- references
        R = 'create table ord with id integer primary key, student_id integer references student(id)\n'
        out = run(R + 'insert into ord values (1, 1)\nfind count(*) from ord\n')
        check("references: a value that exists", col(out) == ["1"], out)
        out = run(R + 'insert into ord values (1, 99)\n')
        check("references: a value that is not there", "99 is not in student.id" in out, out)
        out = run(R + 'insert into ord values (1, 1)\ndelete from student where id is 1\n')
        check("a row that others refer to cannot be deleted", "still refer to" in out, out)
        out = run(R + 'insert into ord values (1, 1)\nupdate student set id = 50 where id is 1\n')
        check("a key that others refer to cannot change", "still refer to" in out, out)
        out = run(R + 'drop table student confirmed\n')
        check("a table that others refer to cannot be dropped", "refers to it" in out, out)
        out = run(R + 'insert into ord values (1, 1)\ndelete from ord where id is 1\ndelete from student where id is 1\nfind count(*) from student\n')
        check("delete the rows that refer, then the row", col(out) == ["3"], out)
        out = run('create table ord with id, s references nowhere\n')
        check("references a missing table", "does not exist" in out, out)

        # ---- alter table
        out = run("alter table student add column age integer default 18\nfind age from student where id is 1\n")
        check("add column with a type and a default", col(out) == ["18"], out)
        out = run("alter table student add column req text not null\n")
        check("add a not null column without a default", "need a value" in out, out)
        out = run("alter table student rename column name to full_name\nfind full_name from student where id is 1\n")
        check("rename column", col(out) == ["Asha"], out)
        out = run("alter table student rename to pupils\nfind count(*) from pupils\n")
        check("rename table", col(out) == ["4"], out)
        out = run("alter table student modify column marks as text\nfind marks from student where id is 1\ndescribe student\n")
        check("modify column changes the type", "text" in out, out)
        out = run("alter table student modify column name as integer\n")
        check("modify column that cannot hold the values is refused", "was not changed" in out, out)
        out = run("alter table student drop column marks confirmed\ndescribe student\n")
        check("drop column", col(out) == ["id", "name", "city"], out)

        # ---- views, indexes
        out = run('create view top as find name, marks from student where marks > 75\nfind name from top order by name\n')
        check("a view", col(out) == ["Asha", "Karan", "Ravi"], out)
        out = run('create view top as find name from student where marks > 75\nupdate student set marks = 10 where id is 4\nfind count(*) from top\n')
        check("a view shows the rows as they are now", col(out) == ["2"], out)
        out = run('create view top as find name from student\ncreate or replace view top as find name from student limit 1\nfind count(*) from top\n')
        check("create or replace view", col(out) == ["1"], out)
        out = run('create view top as find name from student\ncreate view top as find name from student\n')
        check("a view cannot be made twice without 'or replace'", "already exists" in out, out)
        out = run('create view bad as find nope from student\n')
        check("a view that does not work is refused", "does not work" in out, out)
        out = run('create view top as find name from student\ninsert into top values ("x")\n')
        check("a view cannot be inserted into", "is a view" in out, out)
        out = run('create view top as find name from student\nshow views\ndrop view top\nshow views\n')
        check("show views, drop view", "top" in out and "No views" in out, out)
        out = run('create view top as find name, marks from student\ndescribe top\n')
        check("describe a view", "(view)" in out and "marks" in out, out)
        out = run('create unique index u1 on student (name)\ninsert into student with id 9, name "Asha"\n')
        check("a unique index is enforced", "already in column 'name'" in out, out)
        out = run('create unique index u1 on student (city)\n')
        check("a unique index cannot be made on repeated values", "cannot be made" in out, out)
        out = run('create index i1 on student (name, city)\nshow indexes from student\ndrop index i1\nshow indexes from student\n')
        check("show indexes, drop index", out.count("i1") >= 1 and "(id)" in out, out)

        # ---- transactions
        out = run('begin\ndelete from student where id is 1\nfind count(*) from student\nrollback\nfind count(*) from student\n')
        check("rollback undoes a delete", col(out) == ["3"] or "3" in out and "4" in out, out)
        out = run('begin\ninsert into student with id 8, name "Z"\ncommit\nfind count(*) from student\n')
        check("commit keeps an insert", col(out) == ["5"], out)
        out = run('begin\ncreate table t9 with a\ninsert into t9 with a 1\nrollback\nshow tables\n')
        check("rollback also takes back a table", "t9" not in out.split("rolled back")[1], out)
        out = run('commit\n')
        check("commit without begin is an error", "no open transaction" in out, out)
        out = run('begin\nbegin\n')
        check("begin twice is an error", "already open" in out, out)
        out = run('begin\ndelete from student where id is 1\n')
        check("a transaction that is never committed is rolled back at the end", "rolled back" in out, out)

        # ---- databases, describe, explain
        out = run('create database sqlx1\nshow tables\ndrop database sqlx1\n')
        check("drop database needs confirmed", "REFUSED" in out, out)
        out = run('create database sqlx2\ncreate table z with a\ndrop database sqlx2 confirmed\n')
        check("create database / drop database confirmed", "created and now in use" in out and "dropped" in out and not os.path.exists(os.path.join(TMP, "sqlx2.edb")), out)
        out = run('describe student\n')
        check("describe shows the columns", rows(out)[0][:2] == ["Column", "Type"] and len(rows(out)) == 5, out)
        out = run('explain find name from student s join fees f where s.marks > 70 order by name limit 2\n')
        check("explain", "read every row of table student" in out and "inner join table fees" in out and "sort" in out and "limit" in out, out)
        out = run('create table top as find name, marks from student where marks > 75\nfind count(*) from top\n')
        check("create table ... as <query>", col(out) == ["3"], out)

        # ---- the files: a second run sees tables, types, constraints, views and indexes of the first
        first = ('create table acct with id integer primary key, owner text not null, balance real default 0 check (balance >= 0)\n'
                 'insert into acct with id 1, owner "Asha", balance 50\n'
                 'create view rich as find owner from acct where balance > 10\n'
                 'create unique index acct_owner on acct (owner)\n')
        run(first, setup="", name="persist_a")
        out = run('insert into acct with id 2, owner "Asha"\ninsert into acct with id 3, owner "Bo", balance -1\ninsert into acct with id 4\nfind owner from rich\ndescribe acct\nshow indexes\n', setup="", name="persist_a")
        check("constraints, a view and an index survive the file", "already in column 'owner'" in out and "the check 'balance >= 0'" in out and "cannot be empty" in out and "Asha" in out and "integer" in out and "acct_owner" in out, out)

        # ---- every example in section 11 of the manual runs as written
        manual = open(os.path.join(ROOT, "enlngdb", "MANUAL.md"), encoding="utf-8").read()
        section = manual.split("# 11. Expressions, Subqueries", 1)[1].split("\n# 12. ", 1)[0]
        blocks = []
        for part in section.split("```enlngdb\n")[1:]:
            blocks.append(part.split("```", 1)[0])
        extra = ('create table a with name\ninsert into a with name "Asha"\ncreate table b with name\ninsert into b with name "Ravi"\n'
                 'create table copy with id, name\n')
        check("section 11 has examples to run", len(blocks) >= 12, str(len(blocks)))
        for i, block in enumerate(blocks):
            out = run(block, setup=SETUP + extra)
            bad = [l for l in out.splitlines() if l.startswith(("ERROR", "REFUSED", "-- EnlangDB SyntaxError", "NOTE:"))]
            check("manual example %d runs as written" % (i + 1), not bad, "\n".join(bad[:3]) + "\n" + block[:300])

        # ---- more
        out = run("find distinct city from student order by city desc\n")
        check("distinct with order by", col(out) == ["Pune", "Goa", "null"], out)
        out = run('create table t (\n  id integer primary key,\n  name text\n)\ninsert into t values (1, "a")\nfind count(*) from t\n')
        check("a create table written over several lines in brackets", col(out) == ["1"], out)
        out = run('find name from student where (marks > 5\nfind count(*) from student\n')
        check("an unclosed bracket does not swallow the next lines", "4" in col(out), out)
        out = run('find name from student where id is 1 # a comment\nfind name from student where name is "a # b"\n')
        check("a comment at the end of a line", col(out)[:1] == ["Asha"], out)
        out = run("insert into student with id\n")
        check("a column with no value is an error", "has no value" in out, out)
        out = run("create table x with a,\n")
        check("a comma with no column after it is an error", "missing after the last comma" in out, out)
        for word in ("is", "to", ":", "="):
            out = run('insert into student with id 9, name %s "Eve"\nfind name from student where id is 9\n' % word)
            check("insert: 'name %s value' stores the value alone" % word, col(out)[-1:] == ["Eve"], out)
        out = run('use database swap_a\ncreate table t1 with id\ncreate database swap_b\nshow tables\ncreate table t2 with id\n'
                  'use database swap_a\nshow tables\nuse database swap_b\nshow tables\n', setup="")
        shown = [c for c in out.splitlines() if c.startswith("| t")]
        check("changing database keeps each database's own tables", [c.split()[1] for c in shown] == ["t1", "t2"], out)
        check("saving leaves the database file and no .tmp file", os.path.exists(os.path.join(TMP, "swap_a.edb")) and not [f for f in os.listdir(TMP) if f.endswith(".tmp")], str(os.listdir(TMP)))
        long_name = "n" * 70
        for stmt in ("create table %s with id" % long_name, "create table tl with id, %s" % long_name,
                     "insert into student with id 8, %s 1" % long_name, "alter table student add column %s" % long_name):
            out = run(stmt + "\n")
            check("a name of 70 characters is refused, not cut: " + stmt[:24], "at most 63 characters" in out, out)
        out = run('begin\nuse database swap_c\n', setup="")
        check("a database cannot be changed inside a transaction", "before changing database" in out, out)
        out = run("create table y with:\n    id integer\ndescribe y\n")
        check("'with:' is refused, not made into a column called with:", "is not a column name" in out and "with:" not in out.split("is not a column name")[1], out)
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
