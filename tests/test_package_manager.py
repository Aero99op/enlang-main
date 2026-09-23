"""
tests/test_package_manager.py - Unit tests for Sovereign Enlang Package Manager (enlangg pkg)
"""

import unittest
import sys
import os
import shutil
import tempfile
import json

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'tools')))
import enlang_pkg

class TestEnlangPackageManager(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp(prefix="enlang_pkg_test_")
        self.old_cwd = os.getcwd()
        os.chdir(self.temp_dir)

    def tearDown(self):
        os.chdir(self.old_cwd)
        if os.path.exists(self.temp_dir):
            shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_init_command(self):
        class Args:
            name = "my_app"
        ret = enlang_pkg.cmd_init(Args())
        self.assertEqual(ret, 0)
        self.assertTrue(os.path.exists("enlang.json"))
        self.assertTrue(os.path.exists("main.enlng"))

        with open("enlang.json", "r", encoding="utf-8") as f:
            manifest = json.load(f)
        self.assertEqual(manifest["name"], "my_app")
        self.assertEqual(manifest["main"], "main.enlng")

    def test_add_stdlib_dependency(self):
        class InitArgs:
            name = "calc_app"
        enlang_pkg.cmd_init(InitArgs())

        class AddArgs:
            package = "math"
            git = None
            path = None
        ret = enlang_pkg.cmd_add(AddArgs())
        self.assertEqual(ret, 0)

        with open("enlang.json", "r", encoding="utf-8") as f:
            manifest = json.load(f)
        self.assertIn("math", manifest["dependencies"])
        self.assertEqual(manifest["dependencies"]["math"], "stdlib")

        # Verify .enlang_modules exists and contains lib_math.enlng and math.enlng
        modules_dir = os.path.join(self.temp_dir, ".enlang_modules")
        self.assertTrue(os.path.exists(modules_dir))
        self.assertTrue(os.path.exists(os.path.join(modules_dir, "lib_math.enlng")))
        self.assertTrue(os.path.exists(os.path.join(modules_dir, "math.enlng")))

    def test_install_command(self):
        manifest = {
            "name": "reinstall_app",
            "version": "1.0.0",
            "dependencies": {
                "strings": "stdlib"
            }
        }
        with open("enlang.json", "w", encoding="utf-8") as f:
            json.dump(manifest, f)

        class InstallArgs:
            pass
        ret = enlang_pkg.cmd_install(InstallArgs())
        self.assertEqual(ret, 0)

        modules_dir = os.path.join(self.temp_dir, ".enlang_modules")
        self.assertTrue(os.path.exists(os.path.join(modules_dir, "lib_strings.enlng")))

    def test_remove_dependency(self):
        class InitArgs:
            name = "rm_app"
        enlang_pkg.cmd_init(InitArgs())

        class AddArgs:
            package = "fs"
            git = None
            path = None
        enlang_pkg.cmd_add(AddArgs())
        self.assertTrue(os.path.exists(os.path.join(self.temp_dir, ".enlang_modules", "fs.enlng")))

        class RmArgs:
            package = "fs"
        ret = enlang_pkg.cmd_remove(RmArgs())
        self.assertEqual(ret, 0)

        with open("enlang.json", "r", encoding="utf-8") as f:
            manifest = json.load(f)
        self.assertNotIn("fs", manifest["dependencies"])
        self.assertFalse(os.path.exists(os.path.join(self.temp_dir, ".enlang_modules", "fs.enlng")))

    def test_end_to_end_cli_execution(self):
        import subprocess
        exe_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'enlangg.exe'))
        if not os.path.exists(exe_path):
            return

        class InitArgs:
            name = "calc_app"
        enlang_pkg.cmd_init(InitArgs())

        class AddArgs:
            package = "math"
            git = None
            path = None
        enlang_pkg.cmd_add(AddArgs())

        test_script = os.path.join(self.temp_dir, "test_calc.enlng")
        with open(test_script, "w", encoding="utf-8") as f:
            f.write("use \"math\"\nremember num as 5\nshow \"Result: \" + str(square(num))\n")

        p = subprocess.run([exe_path, "run", test_script], capture_output=True, text=True, cwd=self.temp_dir)
        self.assertEqual(p.returncode, 0, f"STDOUT: {p.stdout}\nSTDERR: {p.stderr}")
        self.assertIn("Result: 25", p.stdout)

if __name__ == '__main__':
    unittest.main()
