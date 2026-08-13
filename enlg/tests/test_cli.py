"""Integration tests for the enlg CLI & REPL."""

import unittest
import tempfile
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.cli import run_source, run_file

class TestCLI(unittest.TestCase):

    def test_run_source_execution(self):
        source = 'declare greeting = "Hello enlg"\nprint greeting'
        vm = run_source(source)
        self.assertEqual(vm.environment.get("greeting"), "Hello enlg")

    def test_run_file_execution(self):
        with tempfile.NamedTemporaryFile("w+", delete=False, suffix=".enlg") as tmp:
            tmp.write('declare val = 42')
            tmp_path = tmp.name
            
        try:
            # Test that executing file does not crash
            run_file(tmp_path)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

    def test_repl_state_persistence(self):
        vm = None
        # Line 1: declare x
        vm = run_source('declare x = 100', vm)
        # Line 2: access x
        vm = run_source('declare y = x', vm)
        
        self.assertEqual(vm.environment.get("x"), 100)
        self.assertEqual(vm.environment.get("y"), 100)

if __name__ == '__main__':
    unittest.main()
