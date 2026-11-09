"""enlngdb: timestamps with time zones, JSON and bytes; triggers for a whole statement; names with spaces in
backticks; show databases listing the files; the larger limits; and encrypted database files.

Builds enlngdb from source into a temp dir (ENLNGDB_TEST_CC names another compiler, such as clang for 64 bits).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = "enlngdb.exe" if os.name == "nt" else "enlngdb"
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]
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
        print("FAIL ", name, str(detail)[:900])


def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    subprocess.run([CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe], cwd=ROOT, check=True)
    return exe


def run(body, db=None, env=None):
    _runs[0] += 1
    name = db or "ty%d" % _runs[0]
    script = os.path.join(TMP, "t%d.enlngdb" % _runs[0])
    with open(script, "w", encoding="utf-8") as f:
        f.write("type enlngdb\nuse database %s\n" % name + body)
    e = dict(os.environ)
    e.pop("ENLNGDB_DB_PASSWORD", None)
    e.update(env or {})
    p = subprocess.run([EXE_PATH, script], cwd=TMP, capture_output=True, text=True, encoding="utf-8", errors="replace", env=e)
    return p.stdout + p.stderr


def cells(out):
    vals, row_no = [], None
    for l in out.splitlines():
        if l.startswith("+"):
            row_no = 0 if row_no is None else row_no
            continue
        if l.startswith("|") and row_no is not None:
            if row_no > 0:
                vals += [c.strip() for c in l.strip().strip("|").split("|")]
            row_no += 1
            continue
        row_no = None
    return vals


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-types-")
    try:
        EXE_PATH = build(TMP)

        # ---- timestamps and time zones
        out = run('create table ev with id integer primary key, at timestamp\n'
                  'insert into ev values (1, "2024-03-15 10:30"), (2, "2024-03-15T10:30:00+05:30"), (3, "2024-03-15 23:00:00 -08:00"), (4, "2024-01-31")\n'
                  "find at from ev order by id\n")
        check("timestamps are kept in UTC: no zone (UTC), +05:30, -08:00, a date alone",
              cells(out) == ["2024-03-15 10:30:00Z", "2024-03-15 05:00:00Z", "2024-03-16 07:00:00Z", "2024-01-31 00:00:00Z"], out[-600:])
        out = run('create table ev with id integer primary key, at timestamp\nset time zone "+05:30"\nshow time zone\n'
                  'insert into ev values (1, "2024-03-15 10:30")\nfind at, in_zone(at, "+05:30"), in_zone(at, "UTC") from ev\n')
        check("set time zone: a moment without a zone is read there; in_zone shows it in another",
              cells(out) == ["+05:30", "2024-03-15 05:00:00Z", "2024-03-15 10:30:00+05:30", "2024-03-15 05:00:00Z"], out[-600:])
        out = run('find hour("2024-03-15 10:30:45"), minute("2024-03-15 10:30:45"), second("2024-03-15 10:30:45"), '
                  'date_add("2024-01-31", 1, "month"), date_add("2024-12-31 23:30:00", 45, "minutes"), date_add("2024-03-01", -1, "day"), '
                  'seconds_between("2024-03-15 10:30:00", "2024-03-15T10:30:00+05:30")\n')
        check("hour, minute, second, date_add (month end, across a year, back a day), seconds_between across zones",
              cells(out) == ["10", "30", "45", "2024-02-29", "2025-01-01 00:15:00", "2024-02-29", "19800"], out[-600:])
        out = run('create table ev with at timestamp\ninsert into ev values ("yesterday")\nset time zone "Asia/Kolkata"\n')
        check("a moment that is not one, and a zone name, are refused with a reason",
              "'yesterday' is not one" in out and "Asia/Kolkata' is not a zone" in out, out[-500:])

        # ---- JSON
        out = run('create table doc with id integer primary key, j json\n'
                  "insert into doc values (1, '{\"name\": \"Asha\", \"tags\": [\"a\", \"b\"], \"n\": 3.5, \"ok\": true, \"none\": null, \"s\": \"x\\\\u00e9\"}')\n"
                  "insert into doc values (2, '[1, 2, {\"x\": [10, 20]}]')\n"
                  'find json_get(j, "name"), json_get(j, "tags[1]"), json_get(j, "n") + 1, json_get(j, "ok"), json_type(j), json_length(j), json_get(j, "missing") from doc where id is 1\n'
                  'find json_get(j, "[2].x[1]"), json_length(j, "[2].x"), json_type(j, "[2]") from doc where id is 2\n'
                  "find json_valid('{\"a\": 1}'), json_valid('{a: 1}')\n")
        check("json: get by path (keys, [n], nested), type, length, valid",
              cells(out) == ["Asha", "b", "4.50", "true", "object", "6", "null", "20", "2", "object", "true", "false"], out[-900:])
        out = run("create table doc with j json\ninsert into doc values ('{bad}')\ninsert into doc values ('[1, 2,]')\ninsert into doc values ('\"text\"')\nfind j from doc\n")
        check("json: text that is not JSON is refused, a JSON string is kept",
              out.count("is not JSON") == 2 and cells(out) == ['"text"'], out[-600:])

        # ---- bytes
        out = run('create table f with id integer primary key, b blob\n'
                  'insert into f values (1, "hi"), (2, "0x0aff"), (3, from_base64("AAEC/w=="))\n'
                  'find b, octet_length(b), to_base64(b) from f order by id\nfind hex("AB"), unhex("0x4142")\n')
        check("blob: text as its bytes, 0x hex, base64 in and out, octet_length",
              cells(out) == ["0x6869", "2", "aGk=", "0x0AFF", "2", "Cv8=", "0x000102FF", "4", "AAEC/w==", "0x4142", "AB"], out[-700:])

        # ---- triggers for a whole statement
        out = run('create table item with id integer primary key, price integer\ncreate table log with what text, n integer\n'
                  'create trigger r after update on item do insert into log values ("row", new.id)\n'
                  'create trigger s after update on item for each statement do insert into log values ("statement", count.rows)\n'
                  'create trigger d after delete on item for each statement when count.rows > 1 do insert into log values ("many", count.rows)\n'
                  'insert into item values (1, 10), (2, 20), (3, 30), (4, 40)\n'
                  'update item set price = price + 1 where id < 4\nupdate item set price = 0 where id > 100\n'
                  'delete from item where id is 4\ndelete from item where id < 3\nfind what, n from log\n', db="trg")
        check("a trigger for each statement runs once, with count.rows; not when no row changed; 'when' on count.rows",
              cells(out) == ["row", "1", "row", "2", "row", "3", "statement", "3", "many", "2"], out[-800:])
        out = run('update item set price = 5\nfind what, n from log where what is "statement"\nshow triggers\n', db="trg")
        check("... kept in the file, and shown as 'each statement'",
              cells(out)[:4] == ["statement", "3", "statement", "1"] and "after update, each statement" in out, out[-800:])
        out = run('create table t with id\ncreate trigger b before insert on t for each statement do raise "no"\n')
        check("... a before trigger for each statement is refused", "runs after the statement" in out, out[-300:])

        # ---- names with spaces
        out = run('create table `order details` with id integer primary key, `unit price` real, `product name` text, `order` integer\n'
                  'insert into `order details` values (1, 9.5, "Chai", 3), (2, 4, "Biscuit packet", 10)\n'
                  'find `product name`, `unit price` * `order` as `line total` from `order details` where `unit price` > 3 order by `unit price` desc\n'
                  'update `order details` set `unit price` = 5 where `product name` is "Biscuit packet"\n'
                  'create view `big orders` as find * from `order details` where `order` > 5\nfind `product name` from `big orders`\n'
                  'export `order details` to "od.csv"\nimport "od.csv" into `od copy`\nfind `unit price` from `od copy` where id is 2\n'
                  'show tables\nfind "a `not a name` in text"\ncreate table `bad-name` with x\n', db="names")
        c = cells(out)
        check("names with spaces in backticks: tables, columns, views, a keyword as a name, CSV both ways",
              c[:4] == ["Chai", "28.50", "Biscuit packet", "40.00"] and "Biscuit packet" in c and "5" in c
              and "order details" in c and "big orders" in c and "od copy" in c and "a `not a name` in text" in c
              and "has '-'" in out, out[-1200:])
        check("... the CSV file has the names with their spaces",
              open(os.path.join(TMP, "od.csv"), encoding="utf-8").readline().strip() == "id,unit price,product name,order")

        # ---- show databases
        out = run("show databases\n", db="trg")
        c = cells(out)
        check("show databases lists every database file of the folder (not one never saved), and which is open",
              "trg" in c and "ty1" in c and "names" not in c and c[c.index("trg") + 1] == "yes", out[-800:])

        # ---- the larger limits
        cols = ", ".join("c%d integer" % i for i in range(256))
        body = "create table wide with %s\ninsert into wide (c0, c150, c255) values (1, 150, 255)\n" % cols
        body += "".join("create index ix%d on wide (c%d)\n" % (i, i) for i in range(32))
        body += "create index ix32 on wide (c40)\nupdate wide set c255 = 7 where c150 is 150\nfind c255 from wide\nundo\nfind c0, c150, c255 from wide\n"
        out = run(body)
        check("256 columns and 32 indexes; the 33rd index is refused; update and undo past column 63",
              cells(out) == ["7", "1", "150", "255"] and "at most 32 indexes" in out, out[-600:])

        # ---- encrypted files
        out = run('create table acct with id integer primary key, owner text\ninsert into acct values (1, "Asha"), (2, "Ravi")\n'
                  'encrypt database with password "short"\n', db="sec")
        check("encrypt database: a password of fewer than 8 characters is refused", "at least 8 characters" in out, out[-300:])
        out = run('create table acct with id integer primary key, owner text\ninsert into acct values (1, "Asha"), (2, "Ravi")\n'
                  'encrypt database with password "correct horse"\n', db="sec")
        path = os.path.join(TMP, "sec.edb")
        data = open(path, "rb").read()
        check("the file is encrypted: it starts with ENLNG_C_EDB_ENC1 and holds no text of the rows",
              data[:16] == b"ENLNG_C_EDB_ENC1" and b"Asha" not in data and b"acct" not in data, data[:40])
        out = run("find * from acct\ninsert into acct values (3, \"x\")\n", db="sec")
        check("without the password nothing runs, and the file is not touched",
              "encrypted, and no password was given" in out and "was not opened" in out and open(path, "rb").read() == data, out[-500:])
        _runs[0] += 1
        script = os.path.join(TMP, "t%d.enlngdb" % _runs[0])
        open(script, "w").write('type enlngdb\nuse database sec with password "wrong password"\nfind * from acct\n')
        out = subprocess.run([EXE_PATH, script], cwd=TMP, capture_output=True, text=True).stdout
        check("a wrong password: refused, and the file is not touched", "password of database 'sec' is wrong" in out and open(path, "rb").read() == data, out[-400:])
        open(script, "w").write('type enlngdb\nuse database sec with password "correct horse"\ninsert into acct values (3, "Meera")\nfind owner from acct order by id\n')
        out = subprocess.run([EXE_PATH, script], cwd=TMP, capture_output=True, text=True).stdout
        check("the right password opens it; a change is saved encrypted again",
              cells(out) == ["Asha", "Ravi", "Meera"] and b"Meera" not in open(path, "rb").read(), out[-500:])
        out = run("find owner from acct where id is 3\n", db="sec", env={"ENLNGDB_DB_PASSWORD": "correct horse"})
        check("ENLNGDB_DB_PASSWORD opens it too", cells(out) == ["Meera"], out[-300:])
        changed = bytearray(open(path, "rb").read())
        changed[100] ^= 1
        open(path, "wb").write(bytes(changed))
        out = run("find owner from acct\n", db="sec", env={"ENLNGDB_DB_PASSWORD": "correct horse"})
        check("one byte of the file changed: it is refused, not read wrong", "wrong (or its file was changed)" in out, out[-300:])
        changed[100] ^= 1
        open(path, "wb").write(bytes(changed))
        out = run("decrypt database\n", db="sec", env={"ENLNGDB_DB_PASSWORD": "correct horse"})
        out = run("find count(*) from acct\n", db="sec")
        check("decrypt database: written plain again, and opens without a password",
              cells(out) == ["3"] and b"Meera" in open(path, "rb").read(), out[-300:])
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
    print("%d passed, %d failed" % (passed, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
