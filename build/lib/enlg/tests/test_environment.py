"""Unit tests for the enlg Runtime Environment and Built-ins."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.runtime.environment import Environment
from enlg.runtime.builtins import BUILTINS

class TestEnvironment(unittest.TestCase):

    def test_builtins_loaded(self):
        env = Environment(load_builtins=True)
        # Check if 'length' is loaded and callable
        length_func = env.get("length")
        self.assertTrue(callable(length_func))
        self.assertEqual(length_func([1, 2, 3]), 3)
        self.assertEqual(length_func("hello"), 5)
        self.assertEqual(length_func(123), 0) # Fallback for non-iterables

    def test_variable_scoping(self):
        global_env = Environment()
        global_env.set("x", 10)
        
        local_env = Environment(parent=global_env)
        local_env.set("y", 20)
        
        # Local can access global
        self.assertEqual(local_env.get("x"), 10)
        
        # Local can access local
        self.assertEqual(local_env.get("y"), 20)
        
        # Global cannot access local
        with self.assertRaises(NameError):
            global_env.get("y")
            
        # Local shadowing
        local_env.set("x", 50)
        self.assertEqual(local_env.get("x"), 50)
        self.assertEqual(global_env.get("x"), 10)

if __name__ == '__main__':
    unittest.main()
