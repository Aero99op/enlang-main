"""enlngdb without the old limits: undo on a database of any size (it keeps only what changed), lookups by range
(> < >= <= between) from an index that must give the same rows as reading every row, indexes kept through
deletes, undo and changes to other tables, before triggers, and the || operator.

Builds enlngdb from source into a temp dir and runs small scripts with it.
"""
import os
import random
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
        print("FAIL ", name, str(detail)[:600])


# ENLNGDB_TEST_CC names another compiler (clang for a 64-bit build); the C library of Visual C++ has no libm
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]

def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    subprocess.run([CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe], cwd=ROOT, check=True)
    return exe


def run(body, name=None, env=None):
    _runs[0] += 1
    db = name or "lim%d" % _runs[0]
    script = os.path.join(TMP, "t%d.enlngdb" % _runs[0])
    with open(script, "w", encoding="utf-8") as f:
        f.write("type enlngdb\nuse database %s\n" % db + body)
    p = subprocess.run([EXE_PATH, script], cwd=TMP, capture_output=True, text=True, encoding="utf-8", errors="replace",
                       env=dict(os.environ, **(env or {})))
    return p.stdout + p.stderr


def tables(out):
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
    return [t[1][0] for t in tables(out) if len(t) > 1]


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-limits-")
    try:
        EXE_PATH = build(TMP)

        # ---- undo on a big database: only what changed is kept
        n = 30000
        ins = "\n".join("insert into big values " + ", ".join('(%d, "n%d", %d)' % (i, i, i % 97) for i in range(b, b + 5000)) for b in range(1, n + 1, 5000))
        out = run("create table big with id integer primary key, name text, v integer\n" + ins + "\n"
                  "update big set v = 1000 where id is 777\nundo\nfind v from big where id is 777\n"
                  "delete from big where v < 10\ncount records in big\nundo\ncount records in big\n"
                  "update big set name = \"x\" where v is 5\nundo\nfind count(*) from big where name is \"x\"\n"
                  "truncate table big confirmed\nundo\ncount records in big\n"
                  "alter table big add column extra integer default 7\nundo\ndescribe big\n"
                  "drop table big confirmed\nundo\nfind sum(v) from big\n")
        f = firsts(out)
        expect_sum = sum(i % 97 for i in range(1, n + 1))
        gone = sum(1 for i in range(1, n + 1) if i % 97 < 10)
        check("undo works on 30,000 rows: update, delete, truncate, alter and drop are taken back",
              out.count("Query OK, undone") == 6 and "cannot be undone" not in out and f[0] == str(777 % 97)
              and f[1] == str(n - gone) and f[2] == str(n) and f[3] == "0" and f[4] == str(n) and f[-1] == str(expect_sum)
              and [t for t in tables(out) if t[0][0] == "Column"][0][1:] and "extra" not in [r[0] for r in [t for t in tables(out) if t[0][0] == "Column"][0]], (f, out[-600:]))

        out = run("create table t with id integer primary key, n integer\ncreate table log with id integer primary key auto_increment, k integer\n"
                  "insert into t values (1, 1), (2, 2)\ncreate trigger tl after delete on t do insert into log (k) values (old.id)\n"
                  "drop table t confirmed\nundo\nshow triggers\ndelete from t where id is 1\nfind k from log\n")
        check("undo of drop table brings its triggers back", "tl " in out.split("undone: drop")[1] and firsts(out)[-1] == "1", out[-600:])

        out = run("create table p with id integer primary key, k integer\ncreate table c with id integer primary key, pid integer references p(id) on delete cascade on update cascade\n"
                  "insert into p values (1, 1), (2, 2)\ninsert into c values (10, 1), (11, 1), (12, 2)\n"
                  "update p set id = 5 where id is 1\nundo\nfind pid from c order by id\ndelete from p where id is 2\nundo\ncount records in c\n")
        check("undo takes back what a cascade did in other tables", [r[0] for r in tables(out)[0][1:]] == ["1", "1", "2"] and firsts(out)[-1] == "3", out[-600:])

        out = run("create table t with id integer primary key, n integer\ninsert into t values (1, 1)\n"
                  "update t set n = 2 where id is 1\ninsert into t values (2, 2)\nundo\n")
        check("an insert after a change means that change can no longer be undone", "there is nothing to undo" in out, out[-300:])

        out = run("create table t with id integer primary key, n integer\ninsert into t values (1, 1), (2, 2), (3, 3)\n"
                  "update t set n = 9 where id > 1\ndelete from t where id is 3\nundo\nfind n from t order by id\nundo\nfind n from t order by id\n",
                  env={"ENLNGDB_UNDO_CHECK": "1"})
        check("undo takes back the last change only, once; the self-check finds nothing wrong",
              [r[0] for r in tables(out)[0][1:]] == ["1", "9", "9"] and "there is nothing to undo" in out and "UNDO-CHECK" not in out, out[-500:])

        # ---- transactions keep records of the changes, not copies of the tables
        out = run("create table big with id integer primary key, name text, v integer\n" + ins + "\n"
                  "begin\nupdate big set v = -1 where id is 5\nsavepoint a\ndelete from big where v < 50\ncount records in big\n"
                  "rollback to a\ncount records in big\nupdate big set v = -2 where id is 6\nundo\nfind v from big where id is 6\n"
                  "insert into big values (999999, \"new\", 1)\nrollback\ncount records in big\nfind v from big where id is 5\n"
                  "begin\nupdate big set v = 7 where id is 7\ncommit\nfind v from big where id is 7\n", env={"ENLNGDB_UNDO_CHECK": "1"})
        f = firsts(out)
        kept = n - sum(1 for i in range(1, n + 1) if i % 97 < 50)      # id 5 is -1 by then, so it goes too
        check("a transaction on 30,000 rows: savepoint, rollback to, undo inside it, rollback and commit",
              f[0] == str(kept) and f[1] == str(n) and f[2] == str(6 % 97) and f[3] == str(n) and f[4] == str(5 % 97) and f[5] == "7"
              and "UNDO-CHECK" not in out, (f, out[-500:]))

        # ---- ranges: the same rows as reading every row
        r = random.Random(7)
        vals = []
        for i in range(1, 400):
            vals.append(r.choice(["%d" % r.randint(-50, 300), "%d.5" % r.randint(0, 99), '"%d"' % r.randint(0, 99), '"w%d"' % r.randint(0, 99), "null", "true"]))
        body = "create table m with id integer primary key, k\ninsert into m values " + ", ".join("(%d, %s)" % (i + 1, v) for i, v in enumerate(vals)) + "\n"
        conds = ["k > 250", "k < -40", "k >= 99.5", "k <= 3", "k between 10 and 12", "k > \"w90\"", "k < \"1\"", "k between \"w1\" and \"w2\"",
                 "id > 390", "id between 100 and 103", "k > 280 and k < 290", "300 > id and id > 295"]
        for c in conds:
            body += "find id from m where %s\nfind id from m where (%s) or 1 = 0\n" % (c, c)
        body += "delete from m where id < 200\nupdate m set k = 5 where id is 300\n"
        for c in conds:
            body += "find id from m where %s\nfind id from m where (%s) or 1 = 0\n" % (c, c)
        out = run(body)
        ts = [t for t in tables(out)]
        pairs = list(zip(ts[0::2], ts[1::2]))
        check("range conditions on 399 rows of numbers, texts, nulls and true give the rows that reading every row gives",
              len(pairs) == 2 * len(conds) and all(a == b for a, b in pairs) and any(len(a) > 1 for a, b in pairs), [i for i, (a, b) in enumerate(pairs) if a != b][:5])

        out = run("create table a with id integer primary key, n integer\ncreate table b with id integer\n"
                  "insert into a values " + ", ".join("(%d, %d)" % (i, i % 7) for i in range(1, 301)) + "\n"
                  "find n from a where id is 150\ninsert into b values (1)\nfind n from a where id is 151\ndelete from a where id is 10\n"
                  "find n from a where id is 152\nupdate a set n = 99 where id is 153\nundo\nfind n from a where id is 153\nfind n from a where id > 297\n"
                  "insert into a values (500, 4)\nfind id from a where id >= 300\n")
        check("lookups stay right through inserts elsewhere, deletes, undo and inserts", firsts(out) == ["3", "4", "5", "6", "4", "300"]
              and [x[0] for x in tables(out)[-2][1:]] == ["4", "5", "6"], firsts(out))

        # ---- before triggers
        out = run("create table acct with id integer primary key, owner text, bal integer, mail text unique, note text\n"
                  "insert into acct values (1, \"Asha\", 100, \"a@x\", \"\")\n"
                  "create trigger tidy before insert on acct do set new.owner = upper(new.owner); set new.note = \"new \" || new.owner\n"
                  "create trigger guard before update on acct when new.bal < 0 do raise \"the balance cannot go below zero\"\n"
                  "create trigger stamp before update on acct do set new.note = \"was \" || old.bal\n"
                  "create trigger keep before delete on acct when old.bal > 0 do raise \"close the account first\"\n"
                  "insert into acct values (2, \"ravi\", 50, \"r@x\", null)\nfind owner, note from acct where id is 2\n"
                  "update acct set bal = 70 where id is 1\nfind note from acct where id is 1\n"
                  "update acct set bal = -1 where id is 2\nfind bal from acct where id is 2\n"
                  "delete from acct where id is 1\ncount records in acct\nundo\nfind note from acct where id is 1\n")
        t = tables(out)
        check("before triggers change the new row, refuse a change, and undo takes their change back",
              t[0][1] == ["RAVI", "new RAVI"] and firsts(out)[1] == "was 100" and "the trigger 'guard' stopped this statement: the balance cannot go below zero" in out
              and firsts(out)[2] == "50" and "close the account first" in out and firsts(out)[3] == "2" and firsts(out)[4] == "" , (firsts(out), out[-700:]))
        out = run("create table u with id integer primary key, mail text unique\ninsert into u values (1, \"a\")\n"
                  "create trigger same before insert on u do set new.mail = \"a\"\ninsert into u values (2, \"b\")\ncount records in u\n"
                  "create trigger x1 before insert on u do insert into u values (9, \"z\")\ncreate trigger x2 before delete on u do set new.mail = \"q\"\n"
                  "create table c with id integer primary key, uid integer references u(id)\ncreate trigger x3 before update on u do set new.id = 7\nupdate u set mail = \"m\" where id is 1\n")
        check("a before trigger keeps the rules of the table; it cannot set a delete's row or a key others refer to",
              "breaks a rule of 'u'" in out and firsts(out)[0] == "1" and "has no new row to set" in out
              and "a before trigger cannot change u.id: c.uid refers to it" in out, out[-800:])

        # ---- or, like "abc%" and bare words: the same rows as reading every row
        r2 = random.Random(11)
        body = "create table w with id integer primary key, k, name text\ninsert into w values " + ", ".join(
            '(%d, %s, "%s")' % (i, r2.choice([str(r2.randint(0, 40)), '"%d"' % r2.randint(0, 40), '"w%d"' % r2.randint(0, 40), "null"]),
                                r2.choice(["Asha", "asha k", "Ravi", "ravindra", "Meera", "12ab", "a_b"]) + str(r2.randint(0, 9)))
            for i in range(1, 400)) + "\n"
        conds = ["id is 5 or id is 300", "k is 3 or name like \"ra%\"", "name like \"asha%\"", "name like \"RAV%\"", "name like \"a_b%\"",
                 "name like \"%a\"", "name is Ravi3", "(id < 20 or id > 390) and k is not null", "k between 1 and 4 or name like \"me%\"",
                 "name like \"1%\" or id is 7", "not (id is 5) and id < 9", "k is w7 or k is 7", "name like \"%avi%\"", "name like \"sha\"", "name like \"%ee_a%\"", "name like \"%dra%\" and id > 50", "k like \"%w1%\" or name like \"%ha k%\""]
        for c in conds:
            body += "find id from w where %s\nfind id from w where (%s) or 1 = 0\n" % (c, c)
        body += "delete from w where id < 100\nupdate w set name = \"Ravi3\" where id is 250\n"
        for c in conds:
            body += "find id from w where %s\nfind id from w where (%s) or 1 = 0\n" % (c, c)
        out = run(body)
        ts = tables(out)
        pairs = list(zip(ts[0::2], ts[1::2]))
        check("or, like (at the start or anywhere), and bare words give the rows that reading every row gives",
              len(pairs) == 2 * len(conds) and all(a == b for a, b in pairs) and sum(1 for a, b in pairs if len(a) > 1) >= 12,
              [conds[i % len(conds)] for i, (a, b) in enumerate(pairs) if a != b][:4])

        # ---- before triggers that change other tables
        out = run("create table acct with id integer primary key, owner text\ncreate table log with id integer primary key auto_increment, what text\n"
                  "create table cnt with name text primary key, n integer\ninsert into cnt values (\"logged\", 0), (\"tried\", 0)\n"
                  "create trigger bi before insert on acct do set new.owner = upper(new.owner); insert into log (what) values (\"before \" || new.owner)\n"
                  "create trigger ai after insert on acct do insert into log (what) values (\"after \" || new.owner)\n"
                  "create trigger cl after insert on log do update cnt set n = n + 1 where name is logged\n"
                  "create trigger bd before delete on acct do update cnt set n = n + 1 where name is tried; raise \"no deletes\"\n"
                  "insert into acct values (1, \"asha\"), (2, \"ravi\")\nfind what from log order by id\nfind n from cnt order by name\n"
                  "delete from acct where id is 1\nfind n from cnt order by name\ncount records in acct\n")
        t = tables(out)
        check("a before trigger can change other tables; that runs before the after triggers, and a raise takes it back",
              [x[0] for x in t[0][1:]] == ["before ASHA", "before RAVI", "after ASHA", "after RAVI"] and [x[0] for x in t[1][1:]] == ["4", "0"]
              and "no deletes" in out and [x[0] for x in t[2][1:]] == ["4", "0"] and firsts(out)[-1] == "2", out[-900:])
        out = run("create table a with id integer primary key, n text\ncreate trigger up before insert on a do set new.n = upper(new.n)\n"
                  "create trigger lo after insert on a do update a set n = n where id is new.id\ncreate procedure p (x) as insert into a values (:x, \"q\")\n", name="keepboth")
        out = run("show triggers\nshow procedures\ncall p(3)\nfind n from a\n", name="keepboth")
        check("before and after triggers and procedures are saved and read back", "before insert" in out and "after insert" in out
              and "| p " in out and firsts(out)[-1] == "Q", out[-600:])

        # ---- ||
        out = run("create table p with a text, b integer\ninsert into p values (\"x\", 1), (null, 2)\nfind a || \"-\" || b as t from p\nselect 1 || 2 as j\n")
        check("|| joins texts and numbers, and null stays null", [x[0] for x in tables(out)[0][1:]] == ["x-1", "null"] and firsts(out)[-1] == "12", out[-400:])
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
