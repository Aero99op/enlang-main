"""Unit tests for Hint Keyword + Flexible English Domain Engine in enlg."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.cli import run_source

class TestDomainIntents(unittest.TestCase):

    def test_ai_train_flexible_english(self):
        # Hint keyword: train, Flexible connectors: the, with
        source = 'train "my_model" with "train_dataset"'
        vm = run_source(source)
        self.assertEqual(len(vm.stack), 1)
        self.assertEqual(vm.stack[0], "my_model")

    def test_ai_predict_flexible_english(self):
        # Hint keyword: predict, Flexible connectors: using
        # target="sample_data" (string), model="my_model" (string — no predict method)
        source = 'predict "sample_data" using "my_model"'
        vm = run_source(source)
        self.assertEqual(len(vm.stack), 1)
        self.assertEqual(vm.stack[0], "Predicted(sample_data)")

    def test_sec_encrypt_flexible_english(self):
        # Hint keyword: encrypt, Flexible connectors: using
        source = 'encrypt "secret_payload" using "sha256"'
        vm = run_source(source)
        self.assertEqual(len(vm.stack), 1)
        # SHA256 length is 64 hex chars
        self.assertEqual(len(vm.stack[0]), 64)

    def test_cloud_deploy_flexible_english(self):
        # Hint keyword: deploy, Flexible connectors: using
        source = 'deploy "auth_service" using {"nodes": 3}'
        vm = run_source(source)
        self.assertEqual(len(vm.stack), 1)
        self.assertEqual(vm.stack[0]["status"], "DEPLOYED")

if __name__ == '__main__':
    unittest.main()
