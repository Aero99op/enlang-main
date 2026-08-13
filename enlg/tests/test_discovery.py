"""Unit tests for enlg Hint Discovery Engine."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.parser.discovery import IntentDiscoveryEngine
from enlg.diagnostics.diagnostics import UnknownHintError, IntentAmbiguityError

class TestDiscovery(unittest.TestCase):

    def test_valid_intent_lock(self):
        source = 'declare x to 20'
        tokens = Lexer(source).tokenize()
        
        # Strip newline/EOF for statement testing
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")
        self.assertEqual(locked.hint_token.value, "declare")

    def test_alias_resolution(self):
        source = 'initialize y as 50'
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")

    def test_unknown_hint(self):
        source = 'magically make x 10'
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        with self.assertRaises(UnknownHintError):
            IntentDiscoveryEngine.process_statement(stmt_tokens)

    def test_intent_ambiguity(self):
        # Previously ambiguous ("declare" vs "display"). 
        # Now it simply locks to the first keyword ("declare") and treats the rest as expressions.
        source = "declare flag = display"
        tokens = Lexer(source).tokenize()
        
        # It should lock to DECLARE_VARIABLE and leave 'display' as an identifier token
        locked = IntentDiscoveryEngine.process_statement(tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")

    def test_fallback_assignment(self):
        pass

if __name__ == '__main__':
    unittest.main()
