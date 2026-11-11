"""enlngdb, the first hour of a new user: scripts as Notepad saves them (a UTF-8 mark at the start, UTF-16, CRLF
lines), statements written over several lines, -e that keeps what it changes, tables that stay straight with names
that are not English, a script that stops when its database cannot be opened, and a server that says what is wrong
and can be stopped by its own client.

Builds enlngdb from source into a temp dir (ENLNGDB_TEST_CC names another compiler, such as clang for 64 bits).
"""
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = "enlngdb.exe" if os.name == "nt" else "enlngdb"
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]
passed = failed = 0
TMP = None
EXE_PATH = None


def check(name, cond, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print("PASS ", name)
    else:
        failed += 1
        print("FAIL ", name, str(detail)[:800])


def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    subprocess.run([CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe], cwd=ROOT, check=True)
    return exe


def write_bytes(name, data):
    with open(os.path.join(TMP, name), "wb") as f:
        f.write(data)


def run(*args, env=None, timeout=120):
    e = dict(os.environ)
    e.update(env or {})
    p = subprocess.run([EXE_PATH] + list(args), cwd=TMP, capture_output=True, env=e, timeout=timeout)
    return p.returncode, p.stdout.decode("utf-8", "replace"), p.stderr.decode("utf-8", "replace")


def table_lines(out):
    return [l.rstrip("\r") for l in out.split("\n") if l.startswith(("|", "+"))]


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-firstrun-")
    server = None
    try:
        EXE_PATH = build(TMP)
        script = ('type enlngdb\r\nuse database shop\r\ncreate table pets with id integer primary key, name text, city text\r\n'
                  'insert into pets values (1, "Tom", "Pune"), (2, "राम", "दिल्ली"), (3, "José", "Córdoba"), (4, "日本語", "東京")\r\n'
                  'find count(*) from pets\r\n')

        # ---- 1. scripts as an editor on Windows saves them ----
        write_bytes("bom.enlngdb", b"\xef\xbb\xbf" + script.encode("utf-8"))
        code, out, err = run("bom.enlngdb")
        check("a script with the UTF-8 mark Notepad writes at its start runs", code == 0 and "| 4 " in out and "not understood" not in out + err, (out + err)[-300:])
        write_bytes("u16.enlngdb", b"\xff\xfe" + 'type enlngdb\r\nuse database shop\r\nfind name from pets where id is 2\r\n'.encode("utf-16-le"))
        code, out, err = run("u16.enlngdb")
        check("a UTF-16 script (Notepad's 'Unicode') runs, and finds a name that is not English", code == 0 and "राम" in out, (out + err)[-300:])
        write_bytes("u16be.enlngdb", b"\xfe\xff" + 'type enlngdb\nuse database shop\nfind count(*) from pets\n'.encode("utf-16-be"))
        code, out, err = run("u16be.enlngdb")
        check("... and one with its bytes the other way round", code == 0 and "| 4 " in out, (out + err)[-300:])

        # ---- 2. a table stays straight with names that are not English ----
        write_bytes("all.enlngdb", 'type enlngdb\nuse database shop\nfind id, name, city from pets where id < 4\n'.encode("utf-8"))
        code, out, err = run("all.enlngdb")
        lines = table_lines(out)
        # letters that take one place each: every line of the table has as many letters as its border
        check("the borders of a table line up with accents and Devanagari (letters counted, not bytes)",
              len(lines) == 7 and len(lines[4]) != len(lines[0]) and len(lines[5]) == len(lines[0])
              and all(l.endswith("|") or l.endswith("+") for l in lines), lines)

        # ---- 3. a statement over several lines ----
        write_bytes("multi.enlngdb", ('type enlngdb\nuse database shop\nfind name\n  from pets\n  where id is 1\n     or id is 3\n  order by name\n'
                                       'insert into pets\n  values (5, "Kit", "Goa")\n\nfind count(*)\nfrom pets\n').encode("utf-8"))
        code, out, err = run("multi.enlngdb")
        check("a statement goes on when the next line starts with from, where, or, order by, values ...",
              code == 0 and "José" in out and "Tom" in out and "| 5 " in out and "not understood" not in out + err, (out + err)[-400:])
        write_bytes("two.enlngdb", 'type enlngdb\nuse database shop\nfind count(*) from pets\nfind 7\n'.encode("utf-8"))
        code, out, err = run("two.enlngdb")
        check("... and two statements on two lines stay two", code == 0 and out.count("row(s) in set") == 2, out[-300:])

        # ---- 4. -e keeps what it changes ----
        code, out, err = run("-e", 'use database shop; insert into pets values (6, "Mo", "Agra")')
        code2, out2, err2 = run("-e", "use database shop; find count(*) from pets")
        check("-e with 'use database' saves its change", code == 0 and "| 6 " in out2, (out + out2)[-300:])
        code, out, err = run("-e", "create table lost with id")
        check("-e that changes something without a database says it was not saved, and leaves no file",
              "was not saved" in out and not os.path.exists(os.path.join(TMP, "default.edb")), out[-300:])
        write_bytes("readonly.enlngdb", b"find 1\n")
        run("readonly.enlngdb")
        check("a script that only reads makes no database file", not os.path.exists(os.path.join(TMP, "readonly.edb")))

        # ---- 5. a statement that is not understood ----
        write_bytes("bad.enlngdb", b"type enlngdb\nuse database shop\ntell me everything\n")
        code, out, err = run("bad.enlngdb")
        check("a statement that is not understood names the manual, and the words statements start with",
              code == 1 and "not understood" in err and "preview" in err and "begins with one of" in err and "enlngdb_manual.pdf" in err and "--help" not in err, err[-500:])

        # ---- 6. the server ----
        code, out, err = run("--serve", "nothere", "--port", str(free_port()))
        check("serving a database that is not there says so (not 'it has no users')", code != 0 and "no database 'nothere' here" in err and "no users" not in err, err[-300:])
        run("-e", 'use database shop; create user asha password "secret-pass-1" admin')
        port = free_port()
        server = subprocess.Popen([EXE_PATH, "--serve", "shop", "--port", str(port), "--readers", "0"], cwd=TMP, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
        head = server.stdout.readline() + server.stdout.readline()
        m = re.search(r"fingerprint: ([0-9a-f-]+)", head)
        fp = m.group(1) if m else ""
        env = {"ENLNGDB_PASSWORD": "secret-pass-1"}
        client = ["--connect", str(port), "--user", "asha", "--trust", fp]
        write_bytes("local.enlngdb", b'type enlngdb\nuse database shop\ninsert into pets values (9, "x", "y")\nfind * from pets\n')
        code, out, err = run("local.enlngdb", timeout=60)
        check("a local script on a served database stops at 'use database' (no statement runs on an empty database), and points to --connect",
              code == 1 and "--connect" in out and "rest of the script was not run" in out and "row affected" not in out and "does not exist" not in out, out[-500:])
        write_bytes("c1.enlngdb", b"tell me everything\nfind count(*) from pets\n")
        code, out, err = run(*client, "c1.enlngdb", env=env)
        check("through the server, a statement that is not understood is told to the client", "not understood" in out + err and "tell me everything" in out + err, (out + err)[-400:])
        write_bytes("stop.enlngdb", b"type enlngdb\r\nshutdown server\r\n")
        code, out, err = run(*client, "stop.enlngdb", env=env)
        try:
            server.communicate(timeout=30)
            stopped = True
        except subprocess.TimeoutExpired:
            stopped = False
        check("'shutdown server' from a script file of the client stops the server", stopped and "The server stops" in out, (out + err)[-300:])
        if stopped:
            server = None
    finally:
        if server:
            server.kill()
        shutil.rmtree(TMP, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
