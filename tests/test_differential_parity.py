"""
=====================================================================
  ENLANG SOVEREIGN DUAL-ENGINE DIFFERENTIAL PARITY TEST HARNESS
  Tests exact output and exit status parity between:
  1. Bare-metal Native C99 Compiler (enlng.exe / enlng)
  2. WebAssembly In-Browser Execution Engine (website/enlang_engine.py)
=====================================================================
"""

import os
import sys
import json
import subprocess
import tempfile
import unittest

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT_DIR, "website"))

import enlang_engine

NATIVE_BIN = os.path.join(ROOT_DIR, "enlng.exe" if sys.platform == "win32" else "enlng")
if not os.path.exists(NATIVE_BIN):
    fallback = os.path.join(ROOT_DIR, "enlng" if sys.platform == "win32" else "enlng.exe")
    if os.path.exists(fallback):
        NATIVE_BIN = fallback


class TestDifferentialEngineParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(NATIVE_BIN):
            raise RuntimeError(f"Native binary not found at {NATIVE_BIN}. Build with build.cmd or gcc.")

    def run_on_both_engines(self, code: str):
        """Runs code on both Native C and WebAssembly engines and returns (native_out, wasm_out)."""
        # 1. WebAssembly engine execution
        wasm_res_raw = enlang_engine.execute_enlang_wasm("differential.enlng", code, "enlng")
        try:
            wasm_res = json.loads(wasm_res_raw) if isinstance(wasm_res_raw, str) else wasm_res_raw
        except Exception as e:
            self.fail(f"WebAssembly JSON parse failure: {e}")

        self.assertTrue(wasm_res.get("success"), f"WebAssembly execution failed: {wasm_res}")
        wasm_out = wasm_res.get("output", "").strip().replace("\r\n", "\n")

        # 2. Native C engine execution
        with tempfile.NamedTemporaryFile("w", suffix=".enlng", delete=False, encoding="utf-8") as tf:
            tf.write(code)
            temp_path = tf.name

        try:
            native_run = subprocess.run(
                [NATIVE_BIN, "run", temp_path],
                capture_output=True,
                text=True,
                timeout=12
            )
            self.assertEqual(
                native_run.returncode, 0,
                f"Native execution failed with code {native_run.returncode}:\n{native_run.stderr}"
            )
            native_out = native_run.stdout.strip().replace("\r\n", "\n")
        finally:
            if os.path.exists(temp_path):
                try:
                    os.remove(temp_path)
                except Exception:
                    pass

        return native_out, wasm_out

    def test_string_split_connectors(self):
        """Tests that split with on, by, and with yield identical results across engines."""
        code = """type enlng
s = "alpha:beta:gamma"
p1 = split s on ":"
p2 = split s by ":"
p3 = split s with ":"
show p1[0]
show p2[1]
show p3[2]
"""
        native_out, wasm_out = self.run_on_both_engines(code)
        self.assertEqual(native_out, "alpha\nbeta\ngamma")
        self.assertEqual(wasm_out, "alpha\nbeta\ngamma")
        self.assertEqual(native_out, wasm_out)

    def test_arithmetic_and_loops(self):
        """Tests counter loop and compound assignments across engines."""
        code = """type enlng
sum = 0
for i from 1 to 5 by 1:
    sum increases by i
show "sum is" sum
"""
        native_out, wasm_out = self.run_on_both_engines(code)
        self.assertEqual(native_out, "sum is 15")
        self.assertEqual(wasm_out, "sum is 15")
        self.assertEqual(native_out, wasm_out)

    def test_conditional_branching(self):
        """Tests chained when/otherwise when/otherwise conditional flow."""
        code = """type enlng
score = 85
when score >= 90:
    show "grade A"
otherwise when score >= 80:
    show "grade B"
otherwise:
    show "grade C"
"""
        native_out, wasm_out = self.run_on_both_engines(code)
        self.assertEqual(native_out, "grade B")
        self.assertEqual(wasm_out, "grade B")
        self.assertEqual(native_out, wasm_out)

    def test_string_mutations(self):
        """Tests native positional string replacement across engines."""
        code = """type enlng
remember word as "spandan"
string_replace "a" in word at 0
show word
"""
        native_out, wasm_out = self.run_on_both_engines(code)
        self.assertEqual(native_out, "apandan")
        self.assertEqual(wasm_out, "apandan")
        self.assertEqual(native_out, wasm_out)

    def test_universal_expression_and_literal_parity(self):
        """Tests loops over literals, direct literal indexing, dot-free access, first/last of, and string repetition."""
        code = """total = 0
for x in [10, 20, 30]:
    total increases by x
show "total=" total
r_sum = 0
for i in 1 to 3:
    r_sum increases by i
show "r_sum=" r_sum
show [100, 200, 300][1]
show "Antigravity"[0]
show {"tier": "Sovereign"}["tier"]
show role of {"name": "Enlang", "role": "Architect"}
show first of [5, 10, 15]
show last of [5, 10, 15]
show "abc" * 3
"""
        native_out, wasm_out = self.run_on_both_engines(code)
        expected = "total= 60\nr_sum= 6\n200\nA\nSovereign\nArchitect\n5\n15\nabcabcabc"
        self.assertEqual(native_out, expected)
        self.assertEqual(wasm_out, expected)
        self.assertEqual(native_out, wasm_out)


if __name__ == "__main__":
    unittest.main()

