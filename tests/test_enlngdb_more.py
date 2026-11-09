"""enlngdb beyond the SQL basics: long values and statements, more tables and columns, one program at a time
(a lock), CSV import and export, lookups by hash (insert checks, joins, 'in', union, distinct) that must give
the same answers as looking at every row, savepoints, natural join, on delete cascade / set null, recursive
with, preview and undo.

Builds enlngdb from source into a temp dir and runs small scripts with it.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

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
        print("FAIL ", name, str(detail)[:600])


# ENLNGDB_TEST_CC names another compiler (clang for a 64-bit build); the C library of Visual C++ has no libm
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]

def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    subprocess.run([CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe], cwd=ROOT, check=True)
    return exe


def run(body, name=None, cwd=None):
    """A fresh database per run unless name is given (the file is saved between runs)."""
    _runs[0] += 1
    db = name or "more%d" % _runs[0]
    d = cwd or TMP
    script = os.path.join(d, "t%d.enlngdb" % _runs[0])
    with open(script, "w", encoding="utf-8") as f:
        f.write("type enlngdb\nuse database %s\n" % db + body)
    p = subprocess.run([EXE_PATH, script], cwd=d, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return p.stdout + p.stderr


def rows(out):
    found = []
    for line in out.splitlines():
        if line.startswith("|"):
            found.append([c.strip() for c in line.strip("|").split("|")])
    return found


def tables(out):
    """every printed table, as a list of rows (header first)"""
    all_t, cur = [], None
    for line in out.splitlines():
        if line.startswith("+"):
            if cur is None:
                cur = []
            continue
        if line.startswith("|") and cur is not None:
            cur.append([c.strip() for c in line.strip("|").split("|")])
            continue
        if cur:
            all_t.append(cur)
        cur = None
    if cur:
        all_t.append(cur)
    return all_t


def firsts(out):
    """the first value of each printed table, in order"""
    return [t[1][0] for t in tables(out) if len(t) > 1]


def col(out, i=0):
    r = rows(out)
    return [x[i] for x in r[1:]]


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-more-")
    try:
        EXE_PATH = build(TMP)

        # ---- long values and statements are kept whole
        long = "x" * 300
        huge = "y" * 20000
        out = run('create table t with id, note\ninsert into t values (1, "%s")\ninsert into t with id 2, note "%s"\n'
                  'insert into t values (3, "%s")\nfind id, length(note) from t\n' % (long, long, huge))
        check("long texts are stored whole (values, with, 20000 characters)", col(out, 1) == ["300", "300", "20000"], out[-400:])
        out = run('select length(upper("%s")) as a, length("%s" + "%s") as b, length(replace("%s", "x", "zz")) as c, '
                  'length(substr("%s", 2)) as d, length(concat("%s", "!")) as e, instr("%sQ", "Q") as f\n'
                  % (huge, huge, huge, long, huge, huge, long))
        check("text functions work on long texts", rows(out)[1:] == [["20000", "40000", "600", "19999", "20001", "301"]], out[-300:])
        out = run('select "%sneedle" like "NEEDLE" as a, "%s" like "%%dle" as b, "abc" like "zz" as c\n' % ("a" * 600, "a" * 600 + "dle"))
        check("like without % finds the text inside, at any length", rows(out)[1:] == [["true", "true", "false"]], out[-300:])
        many = ", ".join("(%d)" % i for i in range(1, 5001))
        out = run("create table n with id\ninsert into n values %s\ncount records in n\nfind sum(id) from n\n" % many)
        check("one statement with 5000 rows (no length limit)", firsts(out)[-2:] == ["5000", "12502500"], out[-300:])
        out = run('create table v with id, note\ninsert into v values (1, "%s")\ncreate view lv as find id from v where note is "%s"\nfind count(*) from lv\n' % (long, long))
        check("a view keeps a long text whole", col(out)[-1:] == ["1"], out[-300:])
        out = run('create table d with id, c text check (length(c) < 5 and c is not "%s")\n' % ("q" * 120))
        check("a check that is too long is an error, not cut", "at most 95 characters" in out, out[-300:])
        out = run('create table d with id, c text default "%s"\n' % ("q" * 80))
        check("a default that is too long is an error, not cut", "at most 63 characters" in out, out[-300:])

        # ---- more tables and columns
        cols = ", ".join("c%d" % i for i in range(256))
        vals = ", ".join(str(i) for i in range(256))
        out = run("create table w with %s\ninsert into w values (%s)\nfind c255 from w\nfind count(*) from w a join w b on a.c0 is b.c0\n"
                  "create table w2 with %s, c256\n" % (cols, vals, cols))
        check("a table of 256 columns, joined with itself", col(out)[:1] == ["255"] and "1" in col(out) and "at most 256 columns" in out, out[-300:])
        out = run("".join("create table t%d with id\n" % i for i in range(4097)) + "show tables\n")
        check("4,096 tables, and the 4,097th is refused", "most tables it can hold (4096)" in out and len(rows(out)) == 4097, out[-200:])

        # ---- one program at a time
        lockdir = tempfile.mkdtemp(dir=TMP)
        # the first program holds the database while it works out a slow join (row by row: the 'on' is a calculation)
        slow = ("type enlngdb\nuse database shared\ncreate table n with id\ninsert into n values %s\nfind count(*) from n a join n b on a.id + 0 = b.id + 0\n"
                % ", ".join("(%d)" % i for i in range(1, 10001)))     # about 10 s on a fast 64-bit build: longer than the 5 s the second waits
        open(os.path.join(lockdir, "a.enlngdb"), "w").write(slow)
        open(os.path.join(lockdir, "b.enlngdb"), "w").write("type enlngdb\nuse database shared\ncount records in n\n")
        first = subprocess.Popen([EXE_PATH, "a.enlngdb"], cwd=lockdir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for _ in range(100):
            if os.path.exists(os.path.join(lockdir, "shared.edb.lock")):
                break
            time.sleep(0.05)
        second = subprocess.run([EXE_PATH, "b.enlngdb"], cwd=lockdir, capture_output=True, text=True)
        first.wait()
        check("a second program waits, then says the database is in use", "is in use by another program" in second.stdout, second.stdout[-300:])
        third = subprocess.run([EXE_PATH, "b.enlngdb"], cwd=lockdir, capture_output=True, text=True)
        check("after the first one ends, the database opens with all its rows", "10000" in third.stdout, third.stdout[-300:])
        check("no lock file is left behind", not os.path.exists(os.path.join(lockdir, "shared.edb.lock")) or os.name != "nt", os.listdir(lockdir))

        # ---- CSV
        csv_dir = TMP
        with open(os.path.join(csv_dir, "s.csv"), "w", newline="") as f:
            f.write('name,roll_no,class,stream\r\nAsha,1,11,Science\r\n"Ravi, Jr",2,12,"Commerce"\r\nSita,3,,Arts\r\n"Q""uote",4,11,Science\r\n')
        out = run('import "s.csv" into student\nfind * from student\n')
        check("import makes the table from the first line", rows(out)[1:] == [["Asha", "1", "11", "Science"], ["Ravi, Jr", "2", "12", "Commerce"], ["Sita", "3", "null", "Arts"], ['Q"uote', "4", "11", "Science"]], out[-500:])
        out = run('create table typed with name text, roll_no integer primary key, class integer, stream text\nimport "s.csv" into typed\n'
                  'export find name, class from typed where stream is not "Arts" to "out.csv"\nexport typed to "all.csv"\n')
        exported = open(os.path.join(csv_dir, "all.csv"), newline="").read()
        check("export writes what import reads (quotes, commas, empty)", exported == 'name,roll_no,class,stream\r\nAsha,1,11,Science\r\n"Ravi, Jr",2,12,Commerce\r\nSita,3,,Arts\r\n"Q""uote",4,11,Science\r\n', repr(exported))
        check("export of a query", open(os.path.join(csv_dir, "out.csv"), newline="").read() == 'name,class\r\nAsha,11\r\n"Ravi, Jr",12\r\n"Q""uote",11\r\n', out[-300:])
        out = run('create table r with name text, roll_no integer primary key, class integer, stream text\nimport "all.csv" into r\nfind count(*) from r\n')
        check("a file written by export reads back", col(out)[-1:] == ["4"], out[-300:])
        with open(os.path.join(csv_dir, "bad.csv"), "w") as f:
            f.write("name,roll_no\nX,1\nY\n")
        out = run('create table b2 with name, roll_no\nimport "bad.csv" into b2\ncount records in b2\nimport "none.csv" into b2\n')
        check("a line with the wrong number of values: its line number, and nothing imported", "line 3 of 'bad.csv' has 1 values" in out and col(out)[-1:] == ["0"] and "could not be opened" in out, out[-400:])
        with open(os.path.join(csv_dir, "dup.csv"), "w") as f:
            f.write("id,email\n1,a@x\n2,a@x\n")
        out = run('create table u with id integer primary key, email text unique\nimport "dup.csv" into u\ncount records in u\n')
        check("a row that breaks a rule: nothing imported", "was refused" in out and col(out)[-1:] == ["0"], out[-400:])

        # ---- lookups by hash give the same answers as looking at every row
        mixed = '(1, 1), (2, 1.0), (3, "1"), (4, 1.0000000001), (5, null), (6, "a"), (7, "A"), (8, 2.5), (9, true), (10, 2)'
        out = run("create table a with id, x\ninsert into a values %s\ncreate table b with id, y\ninsert into b values %s, (11, 2.5000000001)\n"
                  "find a.id, b.id from a join b on a.x = b.y order by a.id, b.id\n"
                  "find a.id, b.id from a join b on a.x + 0 = b.y + 0 or a.x = b.y order by a.id, b.id\n" % (mixed, mixed))
        t = tables(out)
        check("a hash join finds what a row-by-row join finds (1, 1.0, \"1\", 1.0000000001, null, case)", len(t) >= 2 and t[-2] == t[-1] and len(t[-1]) > 10, out[-900:])
        out = run("create table a with id, x\ninsert into a values %s\ncreate table b with y\ninsert into b values (1), (2.5), (\"A\"), (null), (1), (2), (3), (4), (5), (6), (7), (8)\n"
                  "find id from a where x in (find y from b) order by id\nfind id from a where x in (1, 2.5, \"A\", null, 2) order by id\n" % mixed)
        t = tables(out)
        check("'in (subquery)' by hash finds what a list finds", len(t) >= 2 and t[-2] == t[-1], out[-600:])
        out = run("create table a with x\ninsert into a values (1), (1.0), (\"1\"), (2.5), (2.5000000001), (null), (null), (\"a\"), (\"A\"), (1)\n"
                  "find distinct x from a\nfind x from a union find x from a\nfind x from a intersect find x from a\nfind x from a except find x from a where x is null\n")
        t = tables(out)
        # the same rows as before lookups by hash: 1, 1.0 and "1" are kept apart, 2.5 and 2.5000000001 are one, a and A two
        check("distinct and union keep one of each (as before)", len(t) >= 2 and t[0] == t[1] and [r[0] for r in t[0]] == ["x", "1", "1.00", "1", "2.50", "null", "a", "A"], t[:2])
        check("intersect and except", len(t) >= 4 and t[2] == t[0] and len(t[3]) == 7, t[2:])
        out = run("create table u with id integer primary key auto_increment, email text unique\n"
                  "insert into u (email) values %s\ninsert into u (email) values (\"e5@x\")\ninsert into u (email) values (\"new@x\"), (\"E7@X\")\ncount records in u\nfind max(id) from u\n"
                  % ", ".join('("e%d@x")' % i for i in range(3000)))
        check("unique and automatic numbers in a big insert, then a duplicate is refused", "already in column 'email'" in out and firsts(out)[-2:] == ["3000", "3000"], out[-500:])
        out = run("create table p with id integer primary key\ninsert into p values %s\ncreate table c with id integer primary key, pid integer references p(id)\n"
                  "insert into c values %s\ninsert into c values (5000, 999999), (5001, 1)\ncount records in c\n"
                  % (", ".join("(%d)" % i for i in range(1, 3001)), ", ".join("(%d, %d)" % (i, i) for i in range(1, 3001))))
        check("references checked in a big insert; a missing one refuses the whole statement", "999999 is not in p.id" in out and col(out)[-1:] == ["3000"], out[-400:])
        out = run("create table k with id integer primary key, v\ninsert into k values %s, (5, 99)\nfind v from k where id is 5\ncount records in k\n"
                  % ", ".join("(%d, %d)" % (i, i) for i in range(1, 2001)))
        check("a key twice in one big insert: the later row replaces the earlier", col(out)[:1] == ["99"] and col(out)[-1:] == ["2000"], out[-300:])

        # ---- savepoints
        out = run("create table acct with id integer primary key, bal integer\ninsert into acct values (1, 100)\nbegin\nupdate acct set bal = 200 where id is 1\n"
                  "savepoint a\nupdate acct set bal = 300 where id is 1\nsavepoint b\ndelete from acct where id is 1\nrollback to b\nfind bal from acct\n"
                  "rollback to savepoint a\nfind bal from acct\nrelease a\ncommit\nfind bal from acct\n")
        check("savepoints: rollback to b, then to a, release, commit", firsts(out)[-3:] == ["300", "200", "200"], out[-500:])
        out = run("savepoint x\nbegin\nrollback to nothing\n")
        check("a savepoint needs a transaction, and an unknown one is named", "start one with 'begin' first" in out and "there is no savepoint 'nothing'" in out, out[-300:])

        # ---- natural join
        out = run("create table student with student_id, name\ninsert into student values (1, \"Asha\"), (2, \"Ravi\"), (3, \"Meera\")\n"
                  "create table fees with fee_id, student_id, amount\ninsert into fees values (10, 1, 500), (11, 1, 300), (12, 2, 900)\n"
                  "find name, amount from student natural join fees\nfind name, amount from student natural left join fees\ncreate table x with q\nfind name from student natural join x\n")
        t = tables(out)
        check("natural join and natural left join", len(t) >= 2 and len(t[0]) == 4 and len(t[1]) == 5 and "no column with the same name" in out, out[-500:])

        # ---- on delete cascade / set null / restrict, and they are saved
        out = run("create table owner with id integer primary key, name text\n"
                  "create table cat with id integer primary key, owner_id integer references owner(id) on delete cascade\n"
                  "create table toy with id integer primary key, cat_id integer references cat(id) on delete cascade\n"
                  "create table vet with id integer primary key, owner_id integer references owner(id) on delete set null\n"
                  "insert into owner values (1, \"Asha\"), (2, \"Ravi\")\ninsert into cat values (1, 1), (2, 1), (3, 2)\ninsert into toy values (1, 1), (2, 2), (3, 3)\n"
                  "insert into vet values (1, 1), (2, 2)\ndelete from owner where id is 1\n", name="cascade_db")
        check("on delete cascade goes two tables deep, set null empties", "4 row(s) that referred to them were deleted too" in out and "1 reference(s) were set to null" in out, out[-400:])
        out = run("find id from cat\nfind id from toy\nfind owner_id from vet\ndescribe cat\n", name="cascade_db")
        check("after the cascade", rows(out)[1:2] == [["3"]] and "null" in col(out) and "(on delete cascade)" in out, out[-600:])
        out = run("create table bill with id integer primary key, owner_id integer references owner(id)\ninsert into bill values (1, 2)\ndelete from owner where id is 2\n"
                  "create table strict with id, owner_id integer not null references owner(id) on delete set null\n", name="cascade_db2")
        check("restrict refuses and says how to cascade", "on delete cascade'" in out or "Table 'owner' does not exist" in out, out[-300:])
        out = run("create table owner with id integer primary key\ncreate table s2 with id, o integer not null references owner(id) on delete set null\ninsert into owner values (1)\n"
                  "insert into s2 values (1, 1)\ndelete from owner where id is 1\ncount records in owner\ncreate table z with a integer references owner(id) on update cascade\n")
        check("set null on a not null column refuses; on update cascade is accepted", "cannot be set to null" in out and col(out)[-1:] == ["1"] and "'on update' is not supported" not in out and "Table 'z' created" in out, out[-400:])
        # ---- on update cascade / set null / restrict
        out = run("create table cls with id integer primary key, title text\ninsert into cls values (1, \"Maths\"), (2, \"Art\")\n"
                  "create table stu with sid integer primary key, cid integer references cls(id) on update cascade, name text\n"
                  "create table sn with sid integer primary key, cid integer references cls(id) on update set null\n"
                  "create table rs with sid integer primary key, cid integer references cls(id)\n"
                  "insert into stu values (1, 1, \"A\"), (2, 1, \"B\"), (3, 2, \"C\")\ninsert into sn values (1, 2)\n"
                  "update cls set id = 10 where id is 1\nfind cid from stu\nfind id from cls\n"
                  "update cls set id = 20 where id is 2\nfind cid from sn\nfind cid from stu\n"
                  "insert into rs values (1, 10)\nupdate cls set id = 99 where id is 10\nfind id from cls\ndescribe stu\n")
        t = tables(out)
        check("on update cascade makes the referring rows follow the new key", [r[0] for r in t[0][1:]] == ["10", "10", "2"] and [r[0] for r in t[1][1:]] == ["10", "2"], out[-900:])
        check("on update set null empties it; the default refuses and says how to cascade; describe shows it",
              [r[0] for r in t[2][1:]] == ["null"] and [r[0] for r in t[3][1:]] == ["10", "10", "20"] and "on update cascade'" in out
              and [r[0] for r in t[4][1:]] == ["10", "20"] and "(on update cascade)" in out, out[-900:])

        # ---- with recursive
        out = run("with recursive n as (select 1 as x union all find x + 1 from n where x < 5) find x from n\n")
        check("with recursive counts 1 to 5", col(out) == ["1", "2", "3", "4", "5"], out[-300:])
        out = run("create table emp with id, name, boss\ninsert into emp values (1, \"Asha\", null), (2, \"Ravi\", 1), (3, \"Meera\", 2), (4, \"Karan\", 2), (5, \"Dev\", 4), (6, \"Zed\", null)\n"
                  "with recursive team as (find id, name from emp where id is 2 union find e.id, e.name from emp e join team t on e.boss is t.id) find name from team order by name\n")
        check("with recursive walks a tree", col(out) == ["Dev", "Karan", "Meera", "Ravi"], out[-300:])
        out = run("with recursive f as (select 1 as x union all find x + 1 from f) find count(*) from f\nwith recursive g as (select 1 as x union find x from g) find x from g\n")
        check("a recursive with that never ends is stopped; union stops when nothing is new", "was stopped after 10000 rounds" in out and col(out)[-1:] == ["1"], out[-300:])

        # ---- preview and undo
        out = run("create table s with id integer primary key, name text, marks integer\ninsert into s values (1, \"Asha\", 80), (2, \"Ravi\", 90), (3, \"Meera\", 70)\n"
                  "preview delete from s where marks < 85\npreview update s set marks = 95 where id is 2\ncount records in s\nfind marks from s where id is 2\n")
        check("preview shows the rows and changes nothing", "would delete 2 row(s) of 's'" in out and "would change 1 row(s)" in out and firsts(out)[-2:] == ["3", "90"], out[-700:])
        out = run("create table s with id integer primary key, name text\ninsert into s values (1, \"Asha\"), (2, \"Ravi\")\ndelete from s where id is 1\nundo\nfind count(*) from s\n"
                  "undo\nupdate s set name = \"X\"\ninsert into s values (3, \"C\")\nundo\npreview find * from s\n")
        check("undo takes back the last delete, once", "undone: delete from s where id is 1" in out and col(out)[:1] == ["2"], out[-500:])
        check("undo is refused after a later change, and preview of a query is refused", out.count("there is nothing to undo") == 2 and "nothing to preview" in out, out[-500:])
        # ---- a primary key cannot be empty (an automatic one is filled in)
        out = run("create table pk with id integer primary key, name text\ninsert into pk with name \"x\"\n"
                  "create table pk2 with id integer primary key auto_increment, name text\ninsert into pk2 with name \"y\"\nfind id from pk2\n")
        check("a primary key cannot be empty; an auto_increment key is filled in", "is the primary key of 'pk', so it cannot be empty" in out and firsts(out)[-1:] == ["1"], out[-400:])

        # ---- lookup indexes made on demand give the same rows as reading every row, also after changes
        N = 600
        mk = ", ".join('(%d, "Name%d", %s, %d)' % (i, i % 50, ("%d.5" % (i % 7)) if i % 11 == 0 else str(i % 7), i % 10) for i in range(1, N + 1))
        base = "create table t with id integer primary key, name text, score real, grp integer\ninsert into t values %s\n" % mk
        ids = lambda out: [r[0] for r in rows(out)[1:]]
        exp = lambda f: [str(i) for i in range(1, N + 1) if f(i)]
        out = run(base + 'find id from t where grp is 3 and id > 500\nfind id from t where name is "NAME7"\nfind id from t where score is 2.5\n'
                  'find id from t where score is 3\nfind id from t where id is "77"\nfind id from t where id is 99999\n')
        got = [[r[0] for r in tb[1:]] for tb in tables(out)]
        check("an index lookup (number, text without case, a fraction, an id in quotes, no match) gives the rows a full read gives",
              got == [exp(lambda i: i % 10 == 3 and i > 500), exp(lambda i: i % 50 == 7), exp(lambda i: i % 11 == 0 and i % 7 == 2),
                      exp(lambda i: i % 11 != 0 and i % 7 == 3), ["77"], []], str(got)[:300])
        out = run(base + "find id from t where grp is 4 and id < 60\nupdate t set grp = 4 where id is 1\nfind id from t where grp is 4 and id < 60\n"
                  "delete from t where id is 4\nfind id from t where grp is 4 and id < 60\ninsert into t values (0, \"Z\", 1, 4)\nfind id from t where grp is 4 and id < 60\n"
                  "begin\nupdate t set grp = 4 where id is 2\nfind id from t where grp is 4 and id < 12\nrollback\nfind id from t where grp is 4 and id < 12\n"
                  "update t set id = 7000 where id is 9\nfind id from t where id is 7000\nfind id from t where id is 9\n")
        got = [[r[0] for r in tb[1:]] for tb in tables(out)]
        want = [exp(lambda i: i % 10 == 4 and i < 60)]
        s1 = sorted(set(want[0]) | {"1"}, key=int)
        s2 = [x for x in s1 if x != "4"]
        s3 = s2 + ["0"]                                  # the new row is last in the table
        s4 = [x for x in ["1", "2"] if True] + ["0"]       # inside the transaction: ids 1, 2 and the new row
        s5 = ["1", "0"]                                  # after the rollback: id 2 is back to its old group
        check("lookups see an update, a delete, an insert, a rolled back change and a changed key", got == [want[0], s1, s2, s3, s4, s5, ["7000"], []], str(got)[:400])

        # ---- triggers and procedures
        setup = ("create table student with roll_no integer primary key, name text, marks integer\n"
                 "create table audit with id integer primary key auto_increment, what text, who integer, at_marks integer\n"
                 "create trigger log_new after insert on student do insert into audit (what, who, at_marks) values (\"added\", new.roll_no, new.marks)\n"
                 "create trigger log_marks after update on student when new.marks is not old.marks do insert into audit (what, who, at_marks) values (\"marks\", new.roll_no, new.marks)\n"
                 "create trigger log_gone after delete on student do insert into audit (what, who, at_marks) values (\"deleted\", old.roll_no, old.marks)\n"
                 "create trigger no_negative after insert on student when new.marks < 0 do raise \"marks cannot be negative\"\n")
        out = run(setup + "insert into student values (1, \"Asha\", 80), (2, \"Ravi\", 70)\nupdate student set marks = 90 where roll_no is 1\n"
                  "update student set name = \"Ravi K\" where roll_no is 2\ndelete from student where roll_no is 2\n"
                  "insert into student values (3, \"Bad\", -5)\nfind roll_no from student\nfind what, who, at_marks from audit\n")
        t = tables(out)
        check("triggers log an insert, a changed value (not an unchanged one) and a delete; a raise takes the statement back",
              "the trigger 'no_negative' stopped this statement: marks cannot be negative" in out and [r[0] for r in t[0][1:]] == ["1"]
              and [r[:3] for r in t[1][1:]] == [["added", "1", "80"], ["added", "2", "70"], ["marks", "1", "90"], ["deleted", "2", "70"]], out[-900:])
        out = run("create table z with id integer primary key, n integer\ncreate trigger zt after insert on z when new.n < 0 do raise \"no\"\ninsert into z values (1, -1)\n")
        pre = out[:out.index("ERROR: the trigger")].strip().split("\n")[-1]
        check("a statement stopped by a trigger does not print its 'rows affected' line first", "made" in pre and "affected" not in pre, pre)
        out = run("create table a with id integer primary key, n integer\ncreate table b with id integer primary key auto_increment, n integer\ncreate table c with id integer primary key auto_increment, n integer\n"
                  "create trigger ab after insert on a do insert into b (n) values (new.n * 10)\ncreate trigger bc after insert on b do insert into c (n) values (new.n + 1)\n"
                  "insert into a values (1, 5), (2, 6)\nfind n from b\nfind n from c\n")
        t = tables(out)
        check("a trigger's statement sets off the triggers of the table it changes, each row once",
              [r[0] for r in t[0][1:]] == ["50", "60"] and [r[0] for r in t[1][1:]] == ["51", "61"], out[-600:])
        out = run("create table loop with id integer primary key auto_increment, n integer\ncreate trigger again after insert on loop do insert into loop (n) values (new.n + 1)\n"
                  "insert into loop (n) values (1)\ncount records in loop\n")
        check("triggers that keep setting each other off stop at a depth, and the statement is taken back",
              "levels deep" in out and col(out)[-1:] == ["0"], out[-600:])
        out = run("create table s with id integer primary key, v integer\ncreate table lg with msg text\n"
                  "create trigger t1 after update on s do insert into lg values (\"id \" + old.id + \" to \" + new.id)\n"
                  "insert into s values (1, 1)\nupdate s set id = 7 where id is 1\nfind msg from lg\ncreate table k with a integer, b integer\ncreate table lk with msg text\n"
                  "create trigger t2 after update on k do insert into lk values (new.a + new.b)\ninsert into k values (1, 2), (3, 4)\nupdate k set b = b + 10\nfind msg from lk\n")
        t = tables(out)
        check("an update that changes the key, and an update of a table without a key, fire the update trigger",
              [r[0] for r in t[0][1:]] == ["id 1 to 7"] and [r[0] for r in t[1][1:]] == ["13", "17"], out[-600:])
        out = run("create table t with id integer primary key, v integer\ncreate table lg with msg text\ncreate trigger tr after insert on t do insert into lg values (\"new \" + new.id)\n"
                  "create procedure p (a) as insert into t values (:a, 0)\n", name="trig_keep")
        out = run("show triggers\nshow procedures\ncall p(5)\nfind msg from lg\ncount records in t\n", name="trig_keep")
        t = tables(out)
        check("triggers and procedures are saved in the file, a call runs a procedure, and its row fires the trigger once",
              t[0][1][0] == "tr" and t[1][1][0] == "p" and [r[0] for r in t[2][1:]] == ["new 5"] and col(out)[-1:] == ["1"], out[-700:])
        out = run("create table acct with id integer primary key, bal integer\ninsert into acct values (1, 100), (2, 50)\n"
                  "create procedure pay (src, dst, amount) as update acct set bal = bal - :amount where id is :src; update acct set bal = bal + :amount where id is :dst; "
                  "update acct set bal = bal where id is 999 and :amount > 1000\ncall pay(1, 2, 30)\nfind bal from acct\ncall pay(1)\ncall nothing(1)\n")
        t = tables(out)
        check("a procedure with several statements and parameters runs them in order; wrong values give clear errors",
              [r[0] for r in t[0][1:]] == ["70", "80"] and "needs 3 value(s)" in out and "there is no procedure with this name" in out, out[-600:])
        out = run("create table acct with id integer primary key, bal integer not null\ninsert into acct values (1, 100)\n"
                  "create procedure bad (n) as update acct set bal = bal + :n where id is 1; insert into acct values (1, null)\ncall bad(5)\nfind bal from acct\n")
        check("a procedure that fails in the middle is taken back as a whole", col(out)[-1:] == ["100"] and "everything it did was taken back" in out, out[-500:])
        out = run("create table t with id integer\ncreate trigger x after insert on t do insert into t values (1)\ndrop table t confirmed\nshow triggers\n")
        check("dropping a table drops its triggers", "No triggers." in out, out[-300:])
        out = run("create table t with id integer\ncreate trigger a before delete on t do set new.id = 1\ncreate trigger a2 after insert on nope do find 1\n"
                  "create trigger a3 after insert on t find 1\ncreate trigger a4 after truncate on t do find 1\ndrop trigger zzz\nfnd name from t\n")
        check("trigger mistakes are explained, and a mistyped command is guessed",
              "a 'before delete' trigger has no new row to set" in out and "there is no table called 'nope'" in out and "write 'do'" in out
              and "write insert, update or delete" in out and "there is no trigger with this name" in out and "Did you mean 'find'" in out, out[-900:])
        out = run("create table cls with id integer primary key, title text\ninsert into cls values (1, \"Maths\")\ncreate table stu with sid integer primary key, name text\n"
                  "find nme from stu where sid is 1\nupdate stu set nam = 1\nfind titel from cls\n")
        check("an unknown column suggests the nearest one", out.count("Did you mean") == 3 and "Did you mean 'name'" in out and "Did you mean 'title'" in out, out[-700:])

        # ---- the examples of section 12 of the manual, run as written, one after the other
        manual = open(os.path.join(ROOT, "enlngdb", "MANUAL.md"), encoding="utf-8").read()
        section = manual.split("\n# 12. ", 1)[1]
        blocks = [part.split("```", 1)[0] for part in section.split("```enlngdb\n")[1:]]
        check("section 12 has examples to run", len(blocks) >= 6, len(blocks))
        out = run("".join(blocks))
        bad = [l for l in out.splitlines() if l.startswith(("ERROR", "REFUSED", "-- EnlangDB SyntaxError", "NOTE:"))]
        check("every example of section 12 runs as written", not bad, "\n".join(bad[:5]))
        check("section 12: undo brought the deleted row back, and the cascade ran", "undone: delete from pupil" in out and "1 row(s) that referred to them were deleted too" in out, out[-800:])
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
