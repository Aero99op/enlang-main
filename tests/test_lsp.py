"""Talks to `enlng lsp` the way an editor does and checks what comes back.

    python tests/test_lsp.py <path-to-enlng>

Also feeds the server many broken versions of a file (cut off at every
line and at many places inside lines) to make sure it never stops.
"""
import json
import subprocess
import sys

SOURCE = """type enlng

function area with width, height:
    check that width is a number
    give width times height

remember total as area(3, 4)
show total
"""


class Client:
    def __init__(self, exe):
        self.p = subprocess.Popen([exe, "lsp"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.next_id = 1

    def send(self, obj):
        body = json.dumps(obj).encode("utf-8")
        self.p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        self.p.stdin.flush()

    def read(self):
        length = None
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise EOFError("the server stopped")
            line = line.strip()
            if not line:
                break
            if line.lower().startswith(b"content-length:"):
                length = int(line.split(b":")[1])
        return json.loads(self.p.stdout.read(length).decode("utf-8"))

    def request(self, method, params):
        i = self.next_id
        self.next_id += 1
        self.send({"jsonrpc": "2.0", "id": i, "method": method, "params": params})
        while True:
            msg = self.read()
            if msg.get("id") == i:
                return msg

    def notify(self, method, params):
        self.send({"jsonrpc": "2.0", "method": method, "params": params})

    def diagnostics(self):
        while True:
            msg = self.read()
            if msg.get("method") == "textDocument/publishDiagnostics":
                return msg["params"]["diagnostics"]


def main():
    import os
    exe = os.path.abspath(sys.argv[1])
    c = Client(exe)
    bad = []

    def check(name, ok, detail=""):
        print(("ok    " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
        if not ok:
            bad.append(name)

    init = c.request("initialize", {"capabilities": {}})
    caps = init["result"]["capabilities"]
    check("initialize", caps["hoverProvider"] and caps["definitionProvider"], caps)
    uri = "file:///tmp/demo.enlng"
    doc = {"textDocument": {"uri": uri}}

    c.notify("textDocument/didOpen", {"textDocument": {"uri": uri, "languageId": "enlng", "version": 1, "text": SOURCE}})
    check("no mistakes in a good file", c.diagnostics() == [])

    hov = c.request("textDocument/hover", {**doc, "position": {"line": 6, "character": 22}})
    check("hover on a function shows its head", "function area with width, height" in hov["result"]["contents"]["value"], hov)
    hov = c.request("textDocument/hover", {**doc, "position": {"line": 7, "character": 1}})
    check("hover on a keyword explains it", "Prints" in hov["result"]["contents"]["value"], hov)
    hov = c.request("textDocument/hover", {**doc, "position": {"line": 1, "character": 0}})
    check("hover on nothing gives null", hov["result"] is None, hov)

    d = c.request("textDocument/definition", {**doc, "position": {"line": 6, "character": 22}})
    check("go to definition", d["result"]["range"]["start"]["line"] == 2, d)
    d = c.request("textDocument/definition", {**doc, "position": {"line": 7, "character": 7}})
    check("go to a name's definition", d["result"]["range"]["start"]["line"] == 6, d)

    comp = c.request("textDocument/completion", {**doc, "position": {"line": 7, "character": 0}})
    labels = [i["label"] for i in comp["result"]]
    check("completion offers names, built-ins and keywords", {"area", "total", "sqrt", "remember"} <= set(labels), labels[:8])

    sym = c.request("textDocument/documentSymbol", doc)
    check("symbols list the function", any(s["name"] == "area" and s["kind"] == 12 for s in sym["result"]), sym)

    # a mistake is reported on its line
    wrong = SOURCE + 'show area("eight", 2)\n'
    c.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 2}, "contentChanges": [{"text": wrong}]})
    diags = c.diagnostics()
    check("a call with a wrong kind is reported before running",
          len(diags) == 1 and diags[0]["range"]["start"]["line"] == 8 and "checks that its input" in diags[0]["message"], diags)

    # a change inside a line does not read the file again; saving does
    c.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 3}, "contentChanges": [{"text": wrong.replace("eight", "nine")}]})
    c.notify("textDocument/didSave", {"textDocument": {"uri": uri}})
    diags = c.diagnostics()
    check("saving reads the file again", len(diags) == 1, diags)

    c.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 4}, "contentChanges": [{"text": 'type enlng\nshow "é 😀" plus\n'}]})
    diags = c.diagnostics()
    check("text with accents and emoji", len(diags) == 1, diags)

    unknown = c.request("workspace/nothing", {})
    check("an unknown request gets an error", "error" in unknown, unknown)

    # the server must survive any half-written file
    pieces = []
    lines = SOURCE.split("\n")
    for i in range(len(lines)):
        pieces.append("\n".join(lines[: i + 1]))
    for n in range(0, len(SOURCE), 3):
        pieces.append(SOURCE[:n])
        pieces.append(SOURCE[n:])
    pieces += ["", "\n\n\n", "function", "function :", "when", "  indented first\n", "\t\t\n", '"unclosed', "[1, 2", "{", 'use "nothing_here"\n',
               "type enlng\nfunction f with a:\n    check that a is a\n", "type enlng\nshow f\"{\"", "type enlng\nrun together:\n    show 1\n",
               "type enlng\nfor each a and in x:\n    show a\n", "type enlng\nremember a and b from\n"]
    for k, text in enumerate(pieces):
        c.notify("textDocument/didOpen", {"textDocument": {"uri": "file:///tmp/f%d.enlng" % k, "text": text}})
        try:
            c.diagnostics()
        except EOFError:
            c.p.wait(timeout=5)
            check("survives piece %d (exit %s): %r" % (k, c.p.returncode, text), False)
            return 1
    check("survives %d broken files" % len(pieces), True)

    c.request("shutdown", None)
    c.notify("exit", None)
    c.p.wait(timeout=10)
    check("exits when told", c.p.returncode == 0, c.p.returncode)
    print("\n%d failed" % len(bad) if bad else "\nall passed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
