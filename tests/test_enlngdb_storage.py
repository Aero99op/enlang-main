"""enlngdb storage: memory past a budget comes from a file, so a database can be larger than the memory a program
may take; a save never leaves a broken or missing file, even when the program is killed in the middle of it;
a script that only reads does not write the file again; and files written by 32-bit and 64-bit programs (the
row count in 4 or in 8 bytes) are read the same.

Builds enlngdb from source into a temp dir (ENLNGDB_TEST_CC names another compiler, such as clang for 64 bits).
"""
import os
import random
import re
import shutil
import struct
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


def env_with(**kw):
    e = dict(os.environ)
    for k in ("ENLNGDB_MEMORY", "ENLNGDB_MEMSTAT", "ENLNGDB_SPILL_DIR"):
        e.pop(k, None)
    e.update({k: v for k, v in kw.items() if v is not None})
    return e


def write(name, text):
    path = os.path.join(TMP, name)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    return path


def run(script_name, env=None, timeout=900):
    p = subprocess.run([EXE_PATH, script_name], cwd=TMP, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", env=env or env_with(), timeout=timeout)
    return p.returncode, p.stdout, p.stderr


def cells(out):
    """the values of the tables printed (not their headings), one per row and column, in order"""
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


def no_times(out):
    return re.sub(r"\([0-9.]+ ms\)", "", out)


# ---- a Windows job that limits the memory a program may commit --------------------------------------------
def run_limited(limit_mb, script_name, env):
    import ctypes
    import ctypes.wintypes as W
    k = ctypes.WinDLL("kernel32", use_last_error=True)

    class IO(ctypes.Structure):
        _fields_ = [(n, ctypes.c_ulonglong) for n in ("r", "w", "o", "rb", "wb", "ob")]

    class BASIC(ctypes.Structure):
        _fields_ = [("a", ctypes.c_longlong), ("b", ctypes.c_longlong), ("LimitFlags", W.DWORD),
                    ("c", ctypes.c_size_t), ("d", ctypes.c_size_t), ("e", W.DWORD), ("f", ctypes.c_size_t),
                    ("g", W.DWORD), ("h", W.DWORD)]

    class EXT(ctypes.Structure):
        _fields_ = [("Basic", BASIC), ("Io", IO), ("ProcessMemoryLimit", ctypes.c_size_t),
                    ("JobMemoryLimit", ctypes.c_size_t), ("PeakProcessMemoryUsed", ctypes.c_size_t),
                    ("PeakJobMemoryUsed", ctypes.c_size_t)]

    class TE(ctypes.Structure):
        _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD), ("th32ThreadID", W.DWORD),
                    ("th32OwnerProcessID", W.DWORD), ("tpBasePri", W.LONG), ("tpDeltaPri", W.LONG), ("dwFlags", W.DWORD)]

    k.CreateJobObjectW.restype = W.HANDLE
    k.OpenProcess.restype = W.HANDLE
    k.OpenThread.restype = W.HANDLE
    k.CreateToolhelp32Snapshot.restype = W.HANDLE
    job = k.CreateJobObjectW(None, None)
    info = EXT()
    info.Basic.LimitFlags = 0x100              # JOB_OBJECT_LIMIT_PROCESS_MEMORY
    info.ProcessMemoryLimit = limit_mb << 20
    k.SetInformationJobObject(W.HANDLE(job), 9, ctypes.byref(info), ctypes.sizeof(info))
    p = subprocess.Popen([EXE_PATH, script_name], cwd=TMP, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         text=True, encoding="utf-8", errors="replace", creationflags=0x4, env=env)   # suspended
    h = k.OpenProcess(0x1F0FFF, False, p.pid)
    k.AssignProcessToJobObject(W.HANDLE(job), W.HANDLE(h))
    snap = k.CreateToolhelp32Snapshot(4, 0)
    te = TE()
    te.dwSize = ctypes.sizeof(te)
    ok = k.Thread32First(W.HANDLE(snap), ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == p.pid:
            th = k.OpenThread(0x0002, False, te.th32ThreadID)
            k.ResumeThread(W.HANDLE(th))
            k.CloseHandle(W.HANDLE(th))
        ok = k.Thread32Next(W.HANDLE(snap), ctypes.byref(te))
    k.CloseHandle(W.HANDLE(snap))
    out, err = p.communicate(timeout=900)
    q = EXT()
    k.QueryInformationJobObject(W.HANDLE(job), 9, ctypes.byref(q), ctypes.sizeof(q), None)
    k.CloseHandle(W.HANDLE(h))
    k.CloseHandle(W.HANDLE(job))
    return p.returncode, out, err, q.PeakProcessMemoryUsed >> 20


# ---- the file: a version 3 file written again as version 2, with the row counts in 4 or 8 bytes ----------
def as_version2(data, count_bytes):
    b = bytearray()
    i = 0

    def take(n, keep=True):
        nonlocal i
        part = data[i:i + n]
        if keep:
            b.extend(part)
        i += n
        return part

    def text():
        n = struct.unpack("<I", take(4))[0]
        take(n)

    assert data[:14] in (b"ENLNG_C_EDB_V3", b"ENLNG_C_EDB_V4")
    v4 = data[:14] == b"ENLNG_C_EDB_V4"
    i = 16
    b.extend(b"ENLNG_C_EDB_V2\0\0")
    take(64)
    if v4:
        take(16, False)                              # the id of the file (version 4)
    ntab = struct.unpack("<i", take(4))[0]
    for _ in range(ntab):
        take(64)
        text()
        ncol = struct.unpack("<i", take(4))[0]
        for _ in range(ncol):
            take(64)
            take(5)
            for _ in range(4):
                text()
            take(8)
        nidx = struct.unpack("<i", take(4))[0]
        for _ in range(nidx):
            text()
            take(1)
            nk = struct.unpack("<i", take(4))[0]
            for _ in range(nk):
                text()
        rows = struct.unpack("<Q", take(8, False))[0]
        total = struct.unpack("<Q", take(8, False))[0]
        width = take(1, False)[0]
        take(7, False)
        b.extend(struct.pack("<I", rows) if count_bytes == 4 else struct.pack("<Q", rows))
        take(total)                                  # the rows are the same in both versions
        take(rows * width, False)                    # the places of the rows
        nstored = struct.unpack("<I", take(4, False))[0]
        ordered = nstored & 0x80000000               # each index is followed by its column in order
        for _ in range(nstored & 0x7FFFFFFF):
            col, n, ns = struct.unpack("<III", take(12, False))
            take(n * 12 + ns * 4, False)
            if ordered:
                take(struct.unpack("<I", take(4, False))[0] * 12, False)     # numbers: value and row
                take(struct.unpack("<I", take(4, False))[0] * 4, False)      # rows with a text
                take(struct.unpack("<I", take(4, False))[0] * 4, False)      # rows with another kind
    b.extend(data[i:])
    return bytes(b)


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-storage-")
    try:
        EXE_PATH = build(TMP)

        # ---- 1. memory from the file gives the same answers as memory from the system ----
        r = random.Random(5)
        n = 30000
        ins = "\n".join("insert into big values " + ", ".join(
            '(%d, "name %d %s", %d, %s)' % (i, i, r.choice(["pune", "goa", "delhi"]), r.randint(0, 999),
                                           "null" if i % 11 == 0 else str(r.randint(1, 50)))
            for i in range(b, min(b + 1000, n + 1))) for b in range(1, n + 1, 1000))
        body = ("type enlngdb\nuse database spill\ncreate table big with id integer primary key, name text, v integer, d integer\n" + ins + "\n"
                "create table dep with k integer primary key, label text\n"
                + "\n".join('insert into dep values (%d, "dep %d")' % (k, k) for k in range(1, 51)) + "\n"
                "find count(*) from big where v < 100\nfind name from big where id is 12345\n"
                "find name from big where name like \"%7 goa%\" order by id limit 5\n"
                "find count(*), sum(v) from big join dep on big.d = dep.k where dep.k < 10\n"
                "update big set v = v + 1 where id > 20000\nfind sum(v) from big\nundo\nfind sum(v) from big\n"
                "begin\ndelete from big where v > 500\nsavepoint s\nupdate big set name = \"x\" where id < 100\nrollback to s\n"
                "count records in big\nrollback\ncount records in big\n"
                "find d, count(*) from big group by d order by d limit 6\n"
                "delete from big where id between 100 and 5000\ncount records in big\n")
        write("spill.enlngdb", body)
        for f in ("spill.edb",):
            if os.path.exists(os.path.join(TMP, f)):
                os.remove(os.path.join(TMP, f))
        _, plain, _ = run("spill.enlngdb", env_with(ENLNGDB_MEMORY="off"))
        shutil.copy(os.path.join(TMP, "spill.edb"), os.path.join(TMP, "spill_plain.edb"))
        os.remove(os.path.join(TMP, "spill.edb"))
        code, spilled, err = run("spill.enlngdb", env_with(ENLNGDB_MEMORY="4M", ENLNGDB_MEMSTAT="1"))
        m = re.search(r"([0-9.]+) MB in memory at most, ([0-9.]+) MB in the file at most", err)
        check("30,000 rows with 4 MB of memory and the rest in the file: the same answers as all in memory",
              code == 0 and no_times(plain) == no_times(spilled) and m and float(m.group(2)) > 10 and float(m.group(1)) <= 4.5,
              (code, err[-300:], [x for x in zip(no_times(plain).splitlines(), no_times(spilled).splitlines()) if x[0] != x[1]][:3]))
        a_bytes = open(os.path.join(TMP, "spill.edb"), "rb").read()
        b_bytes = open(os.path.join(TMP, "spill_plain.edb"), "rb").read()
        same_file = a_bytes[:80] + a_bytes[96:] == b_bytes[:80] + b_bytes[96:]     # all but the random id of each file
        check("... and the same file", same_file)

        # ---- 2. a folder for the file that cannot be used: the engine goes on with the memory of the system ----
        os.remove(os.path.join(TMP, "spill.edb"))
        code, nodir, _ = run("spill.enlngdb", env_with(ENLNGDB_MEMORY="4M", ENLNGDB_SPILL_DIR=os.path.join(TMP, "no", "such", "folder")))
        check("a folder for the file that does not exist: the same answers, from the memory of the system",
              code == 0 and no_times(nodir) == no_times(plain))

        # ---- 3. larger than the memory the program may take (Windows: a job limits it to 200 MB) ----
        if os.name == "nt":
            rows = 1000000
            with open(os.path.join(TMP, "rows.csv"), "w") as f:
                f.write("id,name,city,v\n")
                for i in range(1, rows + 1):
                    f.write("%d,name number %d,city %d of somewhere,%d\n" % (i, i, i % 500, (i * 7919) % 1000003))
            write("lim_make.enlngdb", 'type enlngdb\nuse database lim\nimport "rows.csv" into t\ncount records in t\n'
                  "find count(*) from t where v < 1000\nfind name from t where id is 999999\n")
            write("lim_use.enlngdb", "type enlngdb\nuse database lim\nupdate t set v = -5 where id is 123456\n"
                  'insert into t values (1000001, "last", "x", 7)\nfind count(*) from t where v < 0\n')
            write("lim_check.enlngdb", "type enlngdb\nuse database lim\nfind v from t where id is 123456\ncount records in t\n")
            want_small = str(sum(1 for i in range(1, rows + 1) if (i * 7919) % 1000003 < 1000))
            code, out, err, peak = run_limited(200, "lim_make.enlngdb", env_with(ENLNGDB_MEMORY="off"))
            check("1,000,000 rows, a limit of 200 MB, all in memory: it does not fit", code != 0 and not os.path.exists(os.path.join(TMP, "lim.edb")),
                  (code, peak, out[-200:]))
            code, out, err, peak = run_limited(200, "lim_make.enlngdb", env_with(ENLNGDB_MEMORY="40M", ENLNGDB_MEMSTAT="1"))
            c = cells(out)
            check("... with 40 MB in memory and the rest in the file: imported, counted, found (peak %d MB)" % peak,
                  code == 0 and c == [str(rows), want_small, "name number 999999"] and peak < 200, (code, c, err[-300:]))
            code, out, err, peak = run_limited(200, "lim_use.enlngdb", env_with(ENLNGDB_MEMORY="40M"))
            code2, out2, _, _ = run_limited(200, "lim_check.enlngdb", env_with(ENLNGDB_MEMORY="40M"))
            check("... opened again in 200 MB, changed and saved; opened once more, the change is there",
                  code == 0 and cells(out) == ["1"] and code2 == 0 and cells(out2) == ["-5", str(rows + 1)], (code, cells(out), cells(out2)))
        else:
            print("SKIP  the memory limit (a Windows job object) is checked on Windows")

        # ---- 4. killed while saving: the file is the old one or the new one, never broken or missing ----
        write("mk.enlngdb", "type enlngdb\nuse database c\ncreate table t with id integer primary key, name text, v integer\n" +
              "\n".join("insert into t values " + ", ".join('(%d, "row %d with some text", %d)' % (i, i, i % 7) for i in range(b, b + 2000))
                        for b in range(1, 200001, 2000)) + "\n")
        run("mk.enlngdb")
        write("chk.enlngdb", "type enlngdb\nuse database c\ncount records in t\nfind count(*) from t where v is 99\n")
        write("chg.enlngdb", 'type enlngdb\nuse database c\nupdate t set v = 99 where id < 1000\ninsert into t values (900001, "x", 99)\n')
        write("back.enlngdb", "type enlngdb\nuse database c\ndelete from t where id is 900001\nupdate t set v = 1 where v is 99\n")
        t0 = time.time()
        run("chg.enlngdb")
        full = time.time() - t0
        run("back.enlngdb")
        rr = random.Random(7)
        seen = {"old": 0, "new": 0}
        broken = []
        for i in range(30):
            p = subprocess.Popen([EXE_PATH, "chg.enlngdb"], cwd=TMP, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env_with())
            time.sleep(rr.uniform(full * 0.3, full * 1.05))
            p.kill()
            p.wait()
            state = cells(run("chk.enlngdb")[1])
            if state == ["200000", "0"]:
                seen["old"] += 1
            elif state == ["200001", "1000"]:
                seen["new"] += 1
                run("back.enlngdb")
            else:
                broken.append(state)
                break
        check("killed 30 times at random moments of a change and its save: never broken (%d old, %d new)" % (seen["old"], seen["new"]),
              not broken and seen["old"] + seen["new"] == 30, broken)

        # ---- 5. a script that only reads leaves the file as it is ----
        path = os.path.join(TMP, "c.edb")
        before = open(path, "rb").read()
        old_time = os.path.getmtime(path) - 100
        os.utime(path, (old_time, old_time))
        write("ro.enlngdb", 'type enlngdb\nuse database c\nfind count(*) from t\nshow tables\ndescribe t\nfind name from t where id is 5\ncount records in t\nexport t to "c.csv"\n')
        code, out, _ = run("ro.enlngdb")
        check("a script that only reads (find, show, describe, count, export): the file is not written again",
              code == 0 and os.path.getmtime(path) == old_time and open(path, "rb").read() == before, (code, out[-300:]))
        write("rw.enlngdb", "type enlngdb\nuse database c\nfind count(*) from t\nupdate t set v = 3 where id is 5\n")
        run("rw.enlngdb")
        wal = path + "-wal"
        check("... and one that changes a row writes only the change, to the log beside the file",
              os.path.getmtime(path) == old_time and os.path.exists(wal) and os.path.getsize(wal) < os.path.getsize(path) / 10
              and cells(run("chk.enlngdb")[1])[0] == "200000", (os.path.exists(wal) and os.path.getsize(wal)))

        # ---- 6. a file with the row counts in 8 bytes (written by a 64-bit program before) reads the same ----
        write("fmk.enlngdb", 'type enlngdb\nuse database f\ncreate table a with id integer primary key, name text\n'
              'insert into a values (1, "one"), (2, "two"), (3, "three")\ncreate table empty with x integer\n'
              'create table b with k text, n real, flag boolean\ninsert into b values ("p", 1.5, true), ("q", null, false)\n'
              'create table empty2 with y text\n'
              'create trigger tr after insert on a do insert into b (k, n) values ("t", new.id)\n')
        run("fmk.enlngdb")
        write("frd.enlngdb", "type enlngdb\nuse database f\nfind * from a\ncount records in empty\nfind * from b\n"
              "count records in empty2\nshow triggers\n")
        _, four, _ = run("frd.enlngdb")
        fpath = os.path.join(TMP, "f.edb")
        data = open(fpath, "rb").read()
        check("the file is version 4: rows with their places, the lookup index of the key, and its id", data[:14] == b"ENLNG_C_EDB_V4")
        open(fpath, "wb").write(as_version2(data, 4))
        _, older, _ = run("frd.enlngdb")
        check("a version 2 file (the row counts in 4 bytes, as the 32-bit programs wrote it) reads the same",
              no_times(four) == no_times(older) and "three" in older, older[-400:])
        open(fpath, "wb").write(as_version2(data, 8))
        _, eight, _ = run("frd.enlngdb")
        check("a version 2 file with the row counts in 8 bytes (a 64-bit program before) reads the same",
              no_times(four) == no_times(eight) and "three" in eight, eight[-400:])
        write("fadd.enlngdb", 'type enlngdb\nuse database f\ninsert into a values (4, "four")\n')
        run("fadd.enlngdb")
        _, again, _ = run("frd.enlngdb")
        check("... saved again, it is written as version 4", open(fpath, "rb").read()[:14] == b"ENLNG_C_EDB_V4" and "four" in again, again[-300:])

        # ---- 7. a file opens without reading its rows; a lookup by the key uses the index kept in the file ----
        n = 200000
        with open(os.path.join(TMP, "lz.csv"), "w") as f:
            f.write("id,name,v\n")
            for i in range(1, n + 1):
                f.write("%d,name %d with some words,%d\n" % (i, i, i % 1000))
        write("lz_make.enlngdb", 'type enlngdb\nuse database lz\nimport "lz.csv" into t\n')
        run("lz_make.enlngdb")
        write("lz_one.enlngdb", "type enlngdb\nuse database lz\nfind name, v from t where id is 199999\nfind name from t where id is 7 or id is 8\ncount records in t\n")
        code, out, err = run("lz_one.enlngdb", env_with(ENLNGDB_MEMSTAT="1"))
        m = re.search(r"([0-9.]+) MB in memory at most", err)
        check("200,000 rows: opened, and three found by the key, with the rows left in the file (%s MB)" % (m.group(1) if m else "?"),
              code == 0 and cells(out) == ["name 199999 with some words", "999", "name 7 with some words", "name 8 with some words", "200000"]
              and m and float(m.group(1)) < 12, (code, cells(out), err[-200:]))
        write("lz_all.enlngdb", "type enlngdb\nuse database lz\nfind count(*) from t where v < 10\n")
        code, out, err = run("lz_all.enlngdb", env_with(ENLNGDB_MEMSTAT="1"))
        m2 = re.search(r"([0-9.]+) MB in memory at most", err)
        check("... a query that needs every row reads them all, and gives the same answer (%s MB)" % (m2.group(1) if m2 else "?"),
              cells(out) == ["2000"] and m2 and float(m2.group(1)) > float(m.group(1)) * 2, (cells(out), err[-200:]))
        write("lz_change.enlngdb", "type enlngdb\nuse database lz\nupdate t set id = 999999 where id is 5\nfind name from t where id is 999999\n"
              "find count(*) from t where id is 5\ndelete from t where id is 6\nfind count(*) from t where id is 6\nfind name from t where id is 7\n")
        code, out, err = run("lz_change.enlngdb")
        check("... after a change in the same script, the index of the file is not used: the new key is found, the old one is gone",
              cells(out) == ["name 5 with some words", "0", "0", "name 7 with some words"], out[-500:])
        code, out, err = run("lz_one.enlngdb")
        write("lz_two.enlngdb", "type enlngdb\nuse database lz\nfind name from t where id is 999999\nfind count(*) from t where id is 6\nfind name from t where id is 200000\n")
        code, out, err = run("lz_two.enlngdb")
        check("... saved (the rows never read copied as their bytes) and opened again: the change is there, with a new index",
              cells(out) == ["name 5 with some words", "0", "name 200000 with some words"], out[-500:])
        data = open(os.path.join(TMP, "lz.edb"), "rb").read()
        check("... the file is version 4", data[:14] == b"ENLNG_C_EDB_V4")

        # ---- 8. the log of changes (.edb-wal) ----
        lz = os.path.join(TMP, "lz.edb")
        wal = lz + "-wal"
        size_before = os.path.getsize(lz)
        mtime_before = os.path.getmtime(lz)
        write("w1.enlngdb", 'type enlngdb\nuse database lz\nupdate t set v = -3 where id is 10\ninsert into t values (500000, "added", 1)\n'
              "delete from t where id is 11\n")
        code, out, err = run("w1.enlngdb")
        check("a change of three rows is saved in the log (%d bytes), and the file of %d MB is not written"
              % (os.path.getsize(wal) if os.path.exists(wal) else -1, size_before >> 20),
              code == 0 and os.path.getmtime(lz) == mtime_before and os.path.getsize(wal) < 5000, out[-300:])
        write("w2.enlngdb", "type enlngdb\nuse database lz\nfind v from t where id is 10\nfind name from t where id is 500000\n"
              "find count(*) from t where id is 11\nfind name from t where id is 12\nfind name from t where id is 999999\ncount records in t\n")
        code, out, err = run("w2.enlngdb", env_with(ENLNGDB_MEMSTAT="1"))
        m = re.search(r"([0-9.]+) MB in memory at most", err)
        check("... opened again: the changes are there, found by the index of the file with what the log changed (%s MB)" % (m.group(1) if m else "?"),
              cells(out) == ["-3", "added", "0", "name 12 with some words", "name 5 with some words", "199999"] and m and float(m.group(1)) < 12, (cells(out), err[-200:]))
        for k in range(3):
            write("w3.enlngdb", 'type enlngdb\nuse database lz\nupdate t set v = %d where id is 20\n' % (100 + k))
            run("w3.enlngdb")
        code, out, err = run("w2.enlngdb")
        check("... more saves: the log holds what differs from the file, not each save", os.path.getsize(wal) < 5000 and os.path.getmtime(lz) == mtime_before, os.path.getsize(wal))
        # a damaged log: nothing is opened, and nothing is written
        good_wal = open(wal, "rb").read()
        bad = bytearray(good_wal)
        bad[40] ^= 1
        open(wal, "wb").write(bytes(bad))
        code, out, err = run("w1.enlngdb")
        check("a damaged log: the database is not opened, and neither the file nor the log is written",
              "log of changes" in out and "damaged" in out and open(wal, "rb").read() == bytes(bad) and os.path.getmtime(lz) == mtime_before, out[-400:])
        open(wal, "wb").write(good_wal)
        # a change of the columns writes the full file again, and the log goes
        write("w4.enlngdb", "type enlngdb\nuse database lz\nalter table t add column note text\n")
        run("w4.enlngdb")
        check("a change of the columns: the full file is written again, and the log is removed",
              not os.path.exists(wal) and os.path.getmtime(lz) != mtime_before, os.path.exists(wal))
        code, out, err = run("w2.enlngdb")
        check("... with every change of the log in it", cells(out)[:5] == ["-3", "added", "0", "name 12 with some words", "name 5 with some words"], cells(out))
        # an old log (of the file before) is left alone
        open(wal, "wb").write(good_wal)
        code, out, err = run("w2.enlngdb")
        check("a log of the file before it was written again is not used", cells(out)[:2] == ["-3", "added"] and code == 0, out[-300:])
        os.remove(wal)
        # many changes: past a quarter of the file (at least 4 MB) the full file is written instead
        write("w5.enlngdb", 'type enlngdb\nuse database lz\nupdate t set name = "a much longer name than before, for the log" where v < 600\n')
        mtime2 = os.path.getmtime(lz)
        run("w5.enlngdb")
        check("many rows changed: the full file is written instead of a big log",
              not os.path.exists(wal) and os.path.getmtime(lz) != mtime2, os.path.exists(wal) and os.path.getsize(wal))

        # ---- 9. ranges and 'like "abc%"' use the column in order that the file keeps ----
        n = 120000
        with open(os.path.join(TMP, "rg.csv"), "w") as f:
            f.write("id,name,v\n")
            for i in range(n):
                f.write("%d,name%07d x,%d\n" % (i, (i * 7919) % n, i % 1000))
        write("rg_make.enlngdb", 'type enlngdb\nuse database rg\ncreate table t (id int primary key, name text unique, v int)\nimport "rg.csv" into t\n')
        run("rg_make.enlngdb")
        queries = ("find count(*) from t where id > %d\nfind count(*) from t where id between 1000 and 1040\n"
                   'find count(*) from t where name like "name00001%%"\nfind count(*) from t where name > "name0000500" and name < "name0000520"\n'
                   "find count(*) from t where id <= 9\nfind count(*) from t where id >= 50 and id < 60 and v is 55\n" % (n - 50))
        write("rg_q.enlngdb", "type enlngdb\nuse database rg\n" + queries)
        code, out, err = run("rg_q.enlngdb", env_with(ENLNGDB_MEMSTAT="1"))
        m = re.search(r"([0-9.]+) MB in memory at most", err)
        check("ranges and the start of a text are found in the file's index: only their rows are read (%s MB for %d rows)" % (m.group(1) if m else "?", n),
              cells(out) == ["49", "41", "100", "20", "10", "1"] and m and float(m.group(1)) < 8, (cells(out), err[-200:]))
        write("rg_all.enlngdb", "type enlngdb\nuse database rg\nfind count(*) from t where v < 1\nfind count(*) from t where id > 5\n")
        code, out, err = run("rg_all.enlngdb")
        check("... a column without an index, and a range of most rows, read every row and give the same", cells(out) == ["120", str(n - 6)], cells(out))
        write("rg_ch.enlngdb", 'type enlngdb\nuse database rg\nupdate t set name = "zzz moved" where id is 1010\ndelete from t where id is 1020\n'
              'insert into t values (%d, "name0000100 zz", 3)\nupdate t set id = 1020 where id is 7\n' % (n + 5))
        run("rg_ch.enlngdb")
        code, out, err = run("rg_q.enlngdb", env_with(ENLNGDB_MEMSTAT="1"))
        with_index = cells(out)
        m = re.search(r"([0-9.]+) MB in memory at most", err)
        check("... after changes saved in the log: the index of the file with what the log changed (%s MB)" % (m.group(1) if m else "?"),
              os.path.exists(os.path.join(TMP, "rg.edb-wal")) and with_index[0] == "50" and with_index[1] == "41" and with_index[4] == "9" and m and float(m.group(1)) < 9, (cells(out), err[-200:]))
        write("rg_same.enlngdb", "type enlngdb\nuse database rg\nfind count(*) from t where v < 2000\n" + queries)      # every row is read first
        code, out, err = run("rg_same.enlngdb")
        check("... and the same answers when every row was read before", cells(out)[1:] == with_index and len(with_index) == 6, (cells(out), with_index))
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
    print("%d passed, %d failed" % (passed, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
