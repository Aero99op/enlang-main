"""enlngdb --serve and --connect: users sign in, each statement is checked against what they were granted,
several clients at once (a transaction makes the others wait), undo takes back only one's own change, the
database is saved, and the command line client works.

Builds enlngdb from source into a temp dir, starts a server on a free port and talks to it over TCP.
"""
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = "enlngdb.exe" if os.name == "nt" else "enlngdb"
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
        print("FAIL ", name, str(detail)[:700])


# ENLNGDB_TEST_CC names another compiler (clang for a 64-bit build); the C library of Visual C++ has no libm
CC = os.environ.get("ENLNGDB_TEST_CC", "gcc")
LM = [] if os.name == "nt" and "clang" in CC else ["-lm"]

def build(tmp):
    exe = os.path.join(tmp, EXE)
    src = ["enlngdb/c/main.c", "enlngdb/c/enlngdb.c", "enlngdb/c/enlngdb_parser.c"]
    subprocess.run([CC, "-O2", "-I.", "-Isrc", "-Ienlngdb/c"] + src + LM + ["-o", exe], cwd=ROOT, check=True)
    return exe


def run_local(body, name):
    path = os.path.join(TMP, "local.enlngdb")
    with open(path, "w", encoding="utf-8") as f:
        f.write("type enlngdb\nuse database %s\n" % name + body)
    p = subprocess.run([EXE_PATH, path], cwd=TMP, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return p.stdout + p.stderr


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


class Conn:
    def __init__(self, port):
        self.s = socket.create_connection(("127.0.0.1", port), timeout=30)
        self.buf = b""

    def send(self, word, text):
        data = text.encode("utf-8")
        self.s.sendall(("%s %d\n" % (word, len(data))).encode() + data)

    def answer(self):
        while True:
            nl = self.buf.find(b"\n")
            if nl >= 0:
                word, n = self.buf[:nl].decode().split()
                n = int(n)
                if len(self.buf) >= nl + 1 + n:
                    body = self.buf[nl + 1:nl + 1 + n].decode("utf-8", "replace")
                    self.buf = self.buf[nl + 1 + n:]
                    return word, body
            chunk = self.s.recv(65536)
            if not chunk:
                return None, ""
            self.buf += chunk

    def ask(self, word, text):
        self.send(word, text)
        return self.answer()

    def login(self, user, pw):
        return self.ask("login", "%s\n%s" % (user, pw))

    def run(self, text):
        return self.ask("run", text)

    def close(self):
        try:
            self.send("bye", "")
            self.answer()
        except OSError:
            pass
        self.s.close()


def start_server(name, port, *more, plain=True):
    names = name.split() if isinstance(name, str) else list(name)
    args = [EXE_PATH, "--serve"] + names + ["--port", str(port)] + (["--plain"] if plain else []) + list(more)
    p = subprocess.Popen(args, cwd=TMP, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
    line = p.stdout.readline()
    if not plain and "Serving" in line:
        line += p.stdout.readline()             # the key fingerprint
    return p, line


class SealedConn(Conn):
    """the encrypted protocol, spoken with Python's own cryptography library"""

    def __init__(self, port):
        from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey, X25519PublicKey
        from cryptography.hazmat.primitives import serialization as ser
        import hashlib
        import hmac
        self.s = socket.create_connection(("127.0.0.1", port), timeout=30)
        self.buf = b""
        self.raw = b""
        e = X25519PrivateKey.generate()
        E = e.public_key().public_bytes(ser.Encoding.Raw, ser.PublicFormat.Raw)
        self.s.sendall(b"EDB1" + E)
        got = b""
        while len(got) < 68:
            chunk = self.s.recv(68 - len(got))
            if not chunk:
                raise OSError("closed")
            got += chunk
        S, F = got[4:36], got[36:68]
        self.server_key = S
        dh1 = e.exchange(X25519PublicKey.from_public_bytes(S))
        dh2 = e.exchange(X25519PublicKey.from_public_bytes(F))
        prk = hmac.new(b"enlngdb handshake 1", dh1 + dh2, hashlib.sha256).digest()
        th = hashlib.sha256(b"EDB1" + E + S + F).digest()
        self.k_out = hmac.new(prk, th + b"\x01", hashlib.sha256).digest()
        self.k_in = hmac.new(prk, self.k_out + th + b"\x02", hashlib.sha256).digest()
        self.n_out = self.n_in = 0
        self.last_frame = None
        w, t = self.answer()
        assert w == "ready", (w, t)

    @staticmethod
    def nonce(n):
        return b"\0\0\0\0" + n.to_bytes(8, "little")

    def frame(self, word, text):
        from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
        data = text.encode("utf-8")
        pt = ("%s %d\n" % (word, len(data))).encode() + data
        ct = ChaCha20Poly1305(self.k_out).encrypt(self.nonce(self.n_out), pt, b"")
        self.n_out += 1
        return len(ct).to_bytes(4, "little") + ct

    def send(self, word, text):
        self.last_frame = self.frame(word, text)
        self.s.sendall(self.last_frame)

    def answer(self):
        from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
        while True:
            nl = self.buf.find(b"\n")
            if nl >= 0:
                word, n = self.buf[:nl].decode().split()
                n = int(n)
                if len(self.buf) >= nl + 1 + n:
                    body = self.buf[nl + 1:nl + 1 + n].decode("utf-8", "replace")
                    self.buf = self.buf[nl + 1 + n:]
                    return word, body
            while len(self.raw) >= 4 and len(self.raw) >= 4 + int.from_bytes(self.raw[:4], "little"):
                fl = int.from_bytes(self.raw[:4], "little")
                self.buf += ChaCha20Poly1305(self.k_in).decrypt(self.nonce(self.n_in), self.raw[4:4 + fl], b"")
                self.n_in += 1
                self.raw = self.raw[4 + fl:]
            if self.buf.find(b"\n") >= 0:
                continue
            chunk = self.s.recv(65536)
            if not chunk:
                return None, ""
            self.raw += chunk


def fingerprint_of(key):
    import hashlib
    h = hashlib.sha256(key).hexdigest()[:32]
    return "-".join(h[i:i + 4] for i in range(0, 32, 4))


def main():
    global TMP, EXE_PATH
    TMP = tempfile.mkdtemp(prefix="enlngdb-server-")
    server = None
    try:
        EXE_PATH = build(TMP)
        out = run_local("create table item with id integer primary key, name text, price integer\n", "nousers")
        p, line = start_server("nousers", free_port())
        rest = p.communicate(timeout=30)[0]
        check("a database without users is not served", p.returncode != 0 and "has no users" in line + rest, line + rest)

        out = run_local("create table item with id integer primary key, name text, price integer\n"
                        "create table secret with id integer, note text\n"
                        "insert into item values (1, \"pen\", 10), (2, \"book\", 50)\ninsert into secret values (1, \"salary\")\n"
                        "create table log with id integer primary key auto_increment, what text\n"
                        "create trigger tl after insert on item do insert into log (what) values (new.name)\n"
                        "create procedure cheap (n) as update item set price = :n where id is 1\n"
                        "create view v as find * from secret\n"
                        "create user boss password \"boss-pass-1\" admin\ncreate user asha password \"asha-pass-1\"\ncreate user ravi password \"ravi-pass-1\"\n"
                        "grant select, insert on item to asha\ngrant select on item to ravi\nshow users\n", "shop")
        short = run_local("create user mini password \"short\"\n", "shop")
        check("users are made with a password of 8 or more characters, and the file keeps no password",
              "user 'asha' made" in out and "at least 8 characters" in short
              and b"asha-pass-1" not in open(os.path.join(TMP, "shop.edb"), "rb").read(), out[-500:] + short[-300:])

        port = free_port()
        server, line = start_server("shop", port)
        check("the server starts", "Serving 'shop'" in line, line)

        a = Conn(port)
        w, t = a.login("asha", "wrong-pass")
        check("a wrong password is refused", w == "denied" and "wrong" in t, (w, t))
        w, t = a.run("find * from item")
        check("nothing runs before signing in", w == "denied", (w, t))
        w, t = a.login("ASHA", "asha-pass-1")
        check("the right password signs in (the name in any case)", w == "ok" and "as asha" in t, (w, t))

        w, t = a.run("find name from item where id is 2")
        check("a granted table can be read", w == "ok" and "| book |" in t, t)
        w, t = a.run("find * from secret")
        check("a table that was not granted cannot be read", w == "failed" and "asha may not read 'secret'" in t and "salary" not in t, t)
        w, t = a.run("find name from item where id in (select id from secret)")
        check("a table in a subquery needs the right too", w == "failed" and "may not read 'secret'" in t, t)
        tries = ["find * from \"secret\"", "find * from 'secret'", "with s as (find * from secret) find * from s",
                 "find (select note from secret limit 1) as x from item", "find name from item where 1 = 1 union find note from secret",
                 "find * from item natural join secret", "find * from item, secret", "describe secret", "explain find * from secret",
                 "count records in secret", "find * from v", "update item set name = (select note from secret limit 1) where id is 1",
                 "insert into item select id + 10, note, 1 from secret"]
        leaks = []
        for q in tries:
            w2, t2 = a.run(q)
            if "salary" in t2 or "may not" not in t2:
                leaks.append((q, t2[:200]))
        check("no way around the check: quotes, joins, unions, subqueries, with, views, describe, explain", not leaks, leaks)
        w, t = a.run("insert into item values (3, \"ink\", 5)")
        check("a granted insert runs, and its trigger runs with the rights of its maker", w == "ok" and "1 row affected" in t, t)
        w, t = a.run("update item set price = 1 where id is 1\ndelete from item where id is 1\ntruncate table item confirmed")
        check("update, delete and truncate need their rights", w == "failed" and t.count("asha may not") == 3, t)
        w, t = a.run("create table x with a\ndrop table item confirmed\nalter table item add column c text\ncreate user z password \"zzzzzzzz\"\ngrant all on secret to asha")
        check("tables, users and rights are for an admin", w == "failed" and t.count("only an admin") == 5, t)
        w, t = a.run("insert into fresh with a 1\ninsert into item with id 9, colour \"red\"")
        check("insert ... with cannot make a table or a column for a user who is not an admin",
              w == "failed" and "only an admin may add a table" in t and "only an admin may add one" in t, t)
        w, t = a.run("use database shop\nuse database other\nimport \"x.csv\" into item\nexport item to \"x.csv\"")
        check("the served database only; files on the server only for an admin",
              "Database changed to 'shop'" in t and "serves the database 'shop' only" in t and t.count("only an admin may import or export") == 2, t)
        w, t = a.run("call cheap(1)\nshow users\nshow grants")
        check("call needs execute; show users is for an admin; show grants shows one's own",
              "may not call the procedure 'cheap'" in t and "only an admin may list the users" in t and "| asha | item | select, insert |" in t, t)
        w, t = a.run("alter user ravi password \"xxxxxxxx\"\nalter user asha password \"asha-pass-2\"")
        check("a user changes their own password only", "only an admin may change the password of another user" in t and "password of 'asha' was changed" in t, t)

        boss = Conn(port)
        w, t = boss.login("boss", "boss-pass-1")
        w, t = boss.run("grant update on item to asha\ngrant execute on procedure cheap to asha\nfind what from log")
        check("an admin grants, and sees what the trigger wrote", w == "ok" and "| ink  |" in t, t)
        w, t = a.run("update item set price = 2 where id is 1\ncall cheap(3)\nfind price from item where id is 1")
        check("granted rights work at once", w == "ok" and "| 3     |" in t, t)

        # undo is one's own
        w, t = a.run("update item set price = 7 where id is 2")
        w, t = boss.run("undo")
        check("undo does not take back another user's change", "made by another user" in t, t)
        w, t = a.run("undo\nfind price from item where id is 2")
        check("undo takes back one's own change", "undone" in t and "| 50    |" in t, t)

        # a transaction makes the others wait
        w, t = a.run("begin\nupdate item set price = 100 where id is 2")
        r = Conn(port)
        r.login("ravi", "ravi-pass-1")
        got = {}

        def reader():
            got["t0"] = time.time()
            got["ans"] = r.run("find price from item where id is 2")
            got["t1"] = time.time()

        th = threading.Thread(target=reader)
        th.start()
        time.sleep(1.0)
        waited = th.is_alive()
        a.run("commit")
        th.join(20)
        check("while one user's transaction is open the others wait, then see what it kept",
              waited and got.get("ans", (None, ""))[0] == "ok" and "| 100   |" in got["ans"][1] and got["t1"] - got["t0"] >= 0.9, got)

        w, t = a.run("begin\nupdate item set price = 555 where id is 2")
        a.s.close()                     # gone without commit
        time.sleep(0.5)
        w, t = r.run("find price from item where id is 2")
        check("a transaction left open by a client that goes away is rolled back", "| 100   |" in t, t)

        # many clients at once
        errors = []

        def writer(k):
            try:
                c = Conn(port)
                c.login("boss", "boss-pass-1")
                for i in range(25):
                    w2, t2 = c.run("insert into log (what) values (\"c%d-%d\")" % (k, i))
                    if w2 != "ok":
                        errors.append(t2)
                c.close()
            except Exception as e:      # noqa: BLE001
                errors.append(repr(e))

        ths = [threading.Thread(target=writer, args=(k,)) for k in range(8)]
        for x in ths:
            x.start()
        for x in ths:
            x.join(60)
        w, t = boss.run("find count(*) from log where what like \"c%\"")
        check("8 clients at once, 25 inserts each, nothing lost", not errors and "| 200      |" in t, (errors[:2], t))

        # three wrong passwords end the connection
        z = Conn(port)
        z.login("ravi", "nope-nope-1")
        z.login("ravi", "nope-nope-2")
        z.login("ravi", "nope-nope-3")
        try:
            w, t = z.run("find 1")
        except OSError:
            w, t = None, "closed"
        check("three wrong passwords end the connection", w is None, (w, t))

        # the command line client
        env = dict(os.environ, ENLNGDB_PASSWORD="ravi-pass-1")
        script = os.path.join(TMP, "c.enlngdb")
        with open(script, "w") as f:
            f.write("type enlngdb\nuse database shop\nfind name from item order by id\ndelete from item where id is 1\n")
        p = subprocess.run([EXE_PATH, "--connect", str(port), "--user", "ravi", "--plain", script], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("enlngdb --connect runs a script, and fails when a statement is refused",
              "Signed in to 'shop' as ravi" in p.stdout and "| pen  |" in p.stdout and "ravi may not delete from 'item'" in p.stdout and p.returncode == 1, p.stdout + p.stderr)
        p = subprocess.run([EXE_PATH, "--connect", "127.0.0.1:%d" % port, "--user", "ravi", "--plain"], cwd=TMP, input="find count(*) from item\n",
                           capture_output=True, text=True, env=env)
        check("statements can be piped to the client", p.returncode == 0 and "| 3        |" in p.stdout, p.stdout + p.stderr)

        w, t = boss.run("shutdown server")
        rest = server.communicate(timeout=30)[0]
        server = None
        check("an admin stops the server, and it saves", "The server stopped" in rest, rest)
        out = run_local("find price from item where id is 2\nfind count(*) from log\nshow grants for asha\n", "shop")
        check("what the clients did is in the file, users and rights too", "| 100   |" in out and "| 201      |" in out and "select, insert, update" in out, out[-700:])

        # ---- the crypto itself, against known answers
        p = subprocess.run([EXE_PATH, "--selftest"], capture_output=True, text=True)
        check("the crypto gives the known answers (X25519, ChaCha20-Poly1305, SHA-256, HMAC, PBKDF2)", p.returncode == 0, p.stdout)

        # ---- encrypted, and two databases
        run_local("create table acct with id integer primary key, owner text, bal integer\ninsert into acct values (1, \"Asha\", 10)\n"
                  "create user banker password \"bank-pass-1\" admin\ncreate user asha password \"bank-asha-1\"\ngrant select on acct to asha\n", "bank")
        port = free_port()
        server, line = start_server(["shop", "bank"], port, plain=False)
        check("one server serves two databases, encrypted, and shows its key fingerprint",
              "Serving 'shop' and 'bank'" in line and "Key fingerprint:" in line and os.path.exists(os.path.join(TMP, "enlngdb_server.key")), line)
        e = SealedConn(port)
        shown = line.split("Key fingerprint:")[1].split()[0]
        check("the fingerprint the server shows is the one of the key it uses", fingerprint_of(e.server_key) == shown, (shown, fingerprint_of(e.server_key)))
        w, t = e.login("asha", "asha-pass-2")
        check("with two databases, the login must say which one", w == "denied" and "serves several databases" in t, (w, t))
        e.close()
        e = SealedConn(port)
        w, t = e.ask("login", "bank\nasha\nbank-asha-1")
        check("Python's own crypto library talks to the server: it signs in to 'bank', encrypted", w == "ok" and "to 'bank' as asha" in t and "encrypted" in t, (w, t))
        w, t = e.run("find owner from acct\nfind * from item")
        check("each database has its own tables, users and rights", "| Asha  |" in t and "Table 'item' does not exist" in t, t)
        w, t = e.run("use database shop")
        check("a connection stays in the database it signed in to", "serves the database 'bank' only" in t or "only" in t, t)
        # a changed message is noticed: the server ends the connection
        good = e.frame("run", "find owner from acct")
        bad = good[:20] + bytes([good[20] ^ 1]) + good[21:]
        e.s.sendall(bad)
        try:
            w, t = e.answer()
        except Exception:               # noqa: BLE001
            w, t = None, "closed"
        check("a message changed on the way ends the connection", w is None, (w, t))
        e2 = SealedConn(port)
        e2.ask("login", "bank\nasha\nbank-asha-1")
        e2.run("find owner from acct")
        e2.s.sendall(e2.last_frame)     # the same message again
        try:
            w, t = e2.answer()
        except Exception:               # noqa: BLE001
            w, t = None, "closed"
        check("a message sent again (a replay) ends the connection", w is None, (w, t))
        raw = Conn(port)
        try:
            w, t = raw.login("asha", "bank-asha-1")
        except OSError:
            w, t = None, "closed"
        check("a client that does not encrypt is not answered", w is None, (w, t))

        # the command line client: trust on first use, then the key is checked
        known = os.path.join(TMP, "known_servers")
        env = dict(os.environ, ENLNGDB_PASSWORD="bank-asha-1", ENLNGDB_KNOWN_SERVERS=known)
        q = os.path.join(TMP, "q.enlngdb")
        with open(q, "w") as f:
            f.write("find owner from acct\n")
        p = subprocess.run([EXE_PATH, "--connect", "%d/bank" % port, "--user", "asha", q], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("a new server is not trusted without its fingerprint", p.returncode == 1 and "is new to this computer" in p.stderr and shown in p.stderr, p.stdout + p.stderr)
        p = subprocess.run([EXE_PATH, "--connect", "%d/bank" % port, "--user", "asha", "--trust", "0000-1111-2222-3333-4444-5555-6666-7777", q], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("a wrong fingerprint is refused, and nothing is sent", p.returncode == 1 and "not 0000-1111" in p.stderr and "Asha" not in p.stdout, p.stdout + p.stderr)
        p = subprocess.run([EXE_PATH, "--connect", "%d/bank" % port, "--user", "asha", "--trust", shown, q], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("with the right fingerprint the client signs in and remembers the key", p.returncode == 0 and "| Asha  |" in p.stdout and os.path.exists(known), p.stdout + p.stderr)
        p = subprocess.run([EXE_PATH, "--connect", "%d/bank" % port, "--user", "asha", q], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("next time the known key is enough", p.returncode == 0 and "| Asha  |" in p.stdout, p.stdout + p.stderr)
        b = SealedConn(port)
        b.ask("login", "bank\nbanker\nbank-pass-1")
        b.run("shutdown server")
        server.communicate(timeout=30)
        os.remove(os.path.join(TMP, "enlngdb_server.key"))
        server, line = start_server(["bank"], port, plain=False)       # the same address, a new key
        p = subprocess.run([EXE_PATH, "--connect", "%d" % port, "--user", "asha", q], cwd=TMP, capture_output=True, text=True, env=env, input="")
        check("a server whose key changed is refused (someone may be in between)", p.returncode == 1 and "DIFFERENT key" in p.stderr and "Asha" not in p.stdout, p.stdout + p.stderr)
        b = SealedConn(port)
        b.login("banker", "bank-pass-1")
        b.run("shutdown server")
        server.communicate(timeout=30)
        server = None
        p = subprocess.run([EXE_PATH, "--serve", "bank", "--host", "0.0.0.0", "--plain", "--port", str(free_port())], cwd=TMP, capture_output=True, text=True, timeout=30)
        check("--plain is refused for anything but this computer", p.returncode == 1 and "only for this computer" in p.stderr, p.stdout + p.stderr)

        # ---- readers: requests that only read run in other processes, at the same time
        rows = ", ".join("(%d, %d)" % (i, i % 97) for i in range(1, 4001))
        run_local("create table n with id integer primary key, v integer\ninsert into n values %s\ncreate table secret with id, note\n"
                  "create user boss password \"boss-pass-1\" admin\ncreate user ravi password \"ravi-pass-1\"\ngrant select on n to ravi\n" % rows, "par")
        slow = "find count(*) from n a join n b on a.v + 0 = b.v + 0"        # row by row: some seconds
        for readers in (2, 0):
            port = free_port()
            server, line = start_server("par", port, "--readers", str(readers))
            a = Conn(port)
            a.login("boss", "boss-pass-1")
            b = Conn(port)
            b.login("ravi", "ravi-pass-1")
            time.sleep(1.0)                                     # the readers connect and say hello
            t0 = time.time()
            a.s.sendall(b"run %d\n%s" % (len(slow), slow.encode()))     # sent, its answer not waited for
            time.sleep(0.3)
            word, text = b.run("find v from n where id is 777")
            quick = time.time() - t0
            word_a, text_a = a.answer()
            slow_took = time.time() - t0
            if readers:
                check("with readers, a quick read is answered while another client's slow read runs (%.1f s and %.1f s)" % (quick, slow_took),
                      word == "ok" and "| 1 " in text, (word, text[-200:]))
                check("... the quick one came first", quick < slow_took - 0.5 and "count(*)" in text_a, (quick, slow_took, text_a[-200:]))
                word, text = b.run("find v from n where id is 777\ninsert into n values (5000, 1)")
                check("... a script that also changes is run by the server, with the rights of its user", word == "failed" and "may not insert" in text, text[-300:])
                word, text = a.run("insert into n values (5001, 42)")
                word2, text2 = b.run("find v from n where id is 5001")
                check("... a read right after a change sees it (the file is saved before a reader answers)", word == "ok" and "42" in text2, (text, text2))
                word, text = b.run("find note from secret")
                check("... a reader checks rights too", word == "failed" and ("may not" in text or "no table" in text), text[-200:])
            else:
                check("with --readers 0, the quick read waits for the slow one (%.1f s and %.1f s)" % (quick, slow_took),
                      quick >= slow_took - 0.5, (quick, slow_took))
            a.run("shutdown server")
            server.communicate(timeout=60)
            server = None
        time.sleep(1.0)
        if os.name == "nt":
            left = subprocess.run(["tasklist", "/fi", "imagename eq enlngdb.exe", "/fo", "csv", "/nh"], capture_output=True, text=True).stdout
            mine = [l for l in left.splitlines() if "enlngdb.exe" in l]
            check("the readers end with the server", len(mine) == 0, mine)
    finally:
        if server:
            server.kill()
        shutil.rmtree(TMP, ignore_errors=True)
    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
