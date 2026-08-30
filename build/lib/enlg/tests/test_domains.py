"""Universal Cross-Domain Specification & Syntax Tests for enlg."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.cli import run_source

class TestUniversalDomains(unittest.TestCase):

    def test_natural_english_expressions(self):
        source = 'set score to 10\ndeclare check = score is 10'
        vm = run_source(source)
        self.assertTrue(vm.environment.get("check"))

    def test_repeat_until_loop(self):
        source = 'set score to 1\nwhile score is not 4:\n    set score to score + 1'
        vm = run_source(source)
        self.assertEqual(vm.environment.get("score"), 4.0)

    def test_ai_ml_domain_interop(self):
        # Native math interop simulation for AI/DS
        source = 'import math\nnative math.sqrt with 16'
        vm = run_source(source)
        self.assertEqual(len(vm.stack), 1)
        self.assertEqual(vm.stack[0], 4.0)

    def test_cybersec_domain_interop(self):
        # Hashlib simulation for Cybersecurity payloads
        source = 'import hashlib'
        vm = run_source(source)
        hash_mod = vm.environment.get("hashlib")
        self.assertIsNotNone(hash_mod)

    def test_cloud_domain_interop(self):
        # Async and JSON simulation for Cloud APIs
        source = 'import json\ndeclare config = {"region": "us-east-1"}'
        vm = run_source(source)
        cfg = vm.environment.get("config")
        self.assertEqual(cfg["region"], "us-east-1")

if __name__ == '__main__':
    unittest.main()
