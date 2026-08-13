"""Unit tests for the enlg diagnostic error model."""

import unittest
import sys
import os

# Add root to pythonpath
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.diagnostics.diagnostics import (
    EnlgError,
    UnknownHintError,
    IntentAmbiguityError,
    SecurityError
)
from enlg.diagnostics.error_codes import get_error_message


class TestDiagnostics(unittest.TestCase):

    def test_error_code_retrieval(self):
        """Verify that error messages map correctly to their codes."""
        msg = get_error_message("E2002")
        self.assertIn("Ambiguous intent", msg)

        # Test unknown code fallback
        unknown = get_error_message("E9999")
        self.assertEqual(unknown, "An unknown error occurred.")

    def test_enlg_error_formatting(self):
        """Verify base error formatting includes code, message, and details."""
        err = EnlgError("E1001", details="Found '$'")
        out = str(err)
        self.assertTrue(out.startswith("[E1001]"))
        self.assertIn("Unrecognized character", out)
        self.assertIn("Details: Found '$'", out)

    def test_unknown_hint_error(self):
        """Verify the UnknownHintError hardcodes to E2001."""
        err = UnknownHintError(details="Token 'foo' is not a valid hint.")
        self.assertEqual(err.code, "E2001")
        self.assertIn("Unknown hint keyword", err.message)

    def test_intent_ambiguity_error(self):
        """Verify that IntentAmbiguityError strictly fails closed and formats correctly."""
        err = IntentAmbiguityError(details="Matched DECLARE and ASSIGN simultaneously.")
        self.assertEqual(err.code, "E2002")
        self.assertIn("Ambiguous intent", str(err))

    def test_security_error(self):
        """Verify SecurityError bounds."""
        err = SecurityError(details="Subprocess not permitted.")
        self.assertEqual(err.code, "E5001")


if __name__ == '__main__':
    unittest.main()
