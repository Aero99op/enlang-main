"""
tests/test_lsp_server.py - Unit tests for Sovereign Enlang LSP Server (enlangg lsp)
"""

import unittest
import sys
import os
import json
import io

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'tools')))
import enlang_lsp

class TestEnlangLSPServer(unittest.TestCase):

    def test_diagnostic_missing_colon(self):
        code = "when score >= 90\n    show \"A\"\n"
        diags = enlang_lsp.check_diagnostics(code)
        self.assertTrue(len(diags) > 0)
        self.assertIn("Missing colon", diags[0]["message"])
        self.assertEqual(diags[0]["severity"], 1)

    def test_diagnostic_unclosed_string(self):
        code = "show \"hello world\n"
        diags = enlang_lsp.check_diagnostics(code)
        self.assertTrue(len(diags) > 0)
        self.assertIn("unclosed quotation mark", diags[0]["message"])

    def test_diagnostic_unbalanced_paren(self):
        code = "x = (10 + 20\n"
        diags = enlang_lsp.check_diagnostics(code)
        self.assertTrue(len(diags) > 0)
        self.assertIn("Unclosed parenthesis", diags[0]["message"])

    def test_diagnostic_clean_code(self):
        code = "when score >= 90:\n    show \"Grade A\"\notherwise:\n    show \"Grade B\"\n"
        diags = enlang_lsp.check_diagnostics(code)
        self.assertEqual(len(diags), 0)

    def test_hover_feature(self):
        lsp = enlang_lsp.EnlangLSP()
        uri = "file:///test.enlng"
        lsp.documents[uri] = "when score >= 90:\n    show pair.left\n"

        # Hover on 'when' at line 0, col 2
        res_when = lsp.handle_hover(uri, 0, 2)
        self.assertIsNotNone(res_when)
        self.assertIn("when <condition>:", res_when["contents"]["value"])

        # Hover on 'pair.left' at line 1, col 12
        res_pair = lsp.handle_hover(uri, 1, 12)
        self.assertIsNotNone(res_pair)
        self.assertIn("pair.left", res_pair["contents"]["value"])

    def test_completion_feature(self):
        lsp = enlang_lsp.EnlangLSP()
        uri = "file:///test.enlng"
        res = lsp.handle_completion(uri, 0, 0)
        self.assertIn("items", res)
        labels = [item["label"] for item in res["items"]]
        self.assertIn("remember", labels)
        self.assertIn("freeze", labels)
        self.assertIn("when", labels)
        self.assertIn("for each pair", labels)
        self.assertIn("\"lib_math.enlng\"", labels)

    def test_formatting_feature(self):
        lsp = enlang_lsp.EnlangLSP()
        uri = "file:///test.enlng"
        lsp.documents[uri] = "when (score >= 90):\n  show (x)\n"
        edits = lsp.handle_formatting(uri)
        self.assertEqual(len(edits), 1)
        self.assertIn("when score >= 90:\n    show x\n", edits[0]["newText"])

    def test_lsp_stdio_handshake(self):
        import subprocess
        proc = subprocess.Popen(
            [sys.executable, os.path.join(os.path.dirname(__file__), '..', 'tools', 'enlang_lsp.py')],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        try:
            req = {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}}
            payload = json.dumps(req).encode('utf-8')
            msg = f"Content-Length: {len(payload)}\r\n\r\n".encode('latin1') + payload
            proc.stdin.write(msg)
            proc.stdin.flush()

            line = proc.stdout.readline().decode('latin1').strip()
            self.assertTrue(line.lower().startswith("content-length:"))
            length = int(line.split(":")[1].strip())
            proc.stdout.readline()  # empty separator line
            body = proc.stdout.read(length).decode('utf-8')
            resp = json.loads(body)
            self.assertEqual(resp["id"], 1)
            self.assertEqual(resp["result"]["serverInfo"]["name"], "enlangg-lsp")
        finally:
            proc.terminate()
            proc.wait(timeout=2)
            if proc.stdin: proc.stdin.close()
            if proc.stdout: proc.stdout.close()
            if proc.stderr: proc.stderr.close()

if __name__ == '__main__':
    unittest.main()
