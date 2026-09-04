/*
 * enlng - The Dedicated Core Enlng Programming Language Compiler & Runtime
 * Domain: PURE ENLNG ONLY (Backend / Algorithms / System Logic)
 * Zero Web. Zero Mobile. Zero Domain Overlap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define ENLNG_VERSION "1.0.0-pure-core"

const char* CORE_ENLNG_RUNNER = 
"import sys, os, re\n"
"if hasattr(sys.stdout, 'reconfigure'):\n"
"    sys.stdout.reconfigure(encoding='utf-8', errors='replace')\n"
"true = True; false = False; null = None\n"
"\n"
"# --- Standard Library Built-ins & Test Framework ---\n"
"class TestRunner:\n"
"    def __init__(self):\n"
"        self.passed = 0; self.failed = 0; self.total = 0\n"
"    def describe(self, name):\n"
"        print(f'\\n[TEST SUITE] {name}')\n"
"        print('-' * 60)\n"
"    def assert_equal(self, actual, expected, label=''):\n"
"        self.total += 1\n"
"        if actual == expected:\n"
"            self.passed += 1\n"
"            print(f'   PASS: {label}')\n"
"            return True\n"
"        else:\n"
"            self.failed += 1\n"
"            print(f'   FAIL: {label} (Expected: {expected}, Got: {actual})')\n"
"            return False\n"
"    def assert_true(self, condition, label=''):\n"
"        return self.assert_equal(bool(condition), True, label)\n"
"    def print_test_summary(self):\n"
"        print('=' * 60)\n"
"        print(f'Test Summary: {self.passed} Passed, {self.failed} Failed ({self.total} Total)')\n"
"        print('=' * 60)\n"
"test_runner = TestRunner()\n"
"\n"
"def _smart_input(prompt=''):\n"
"    try: val = input(prompt)\n"
"    except (EOFError, KeyboardInterrupt): return ''\n"
"    try:\n"
"        if '.' in val: return float(val)\n"
"        return int(val)\n"
"    except ValueError: return val\n"
"def ask(prompt=''): return _smart_input(prompt)\n"
"\n"
"def transpile_line(line):\n"
"    indent_len = len(line) - len(line.lstrip(' '))\n"
"    indent = line[:indent_len]\n"
"    trimmed = line.strip()\n"
"    if not trimmed or trimmed.startswith('#'): return line\n"
"    \n"
"    # Domain Header Validation\n"
"    if trimmed.startswith('type '):\n"
"        domain = trimmed.split()[1] if len(trimmed.split()) > 1 else ''\n"
"        if domain != 'enlng':\n"
"            print(f'[DOMAIN ERROR] enlng runtime received domain \"{domain}\". Expected \"type enlng\".', file=sys.stderr)\n"
"            print(f'Use the dedicated domain compiler for {domain}.', file=sys.stderr)\n"
"            sys.exit(1)\n"
"        return f'{indent}# [DOMAIN VERIFIED: enlng core]'\n"
"        \n"
"    if trimmed.startswith('hint '):\n"
"        return f'{indent}# [HINT CONTRACT] {trimmed[5:]}'\n"
"\n"
"    def fix_expr(expr):\n"
"        expr = re.sub(r'\\bis equal to\\b', '==', expr)\n"
"        expr = re.sub(r'\\bis not equal to\\b', '!=', expr)\n"
"        expr = re.sub(r'\\bis at least\\b', '>=', expr)\n"
"        expr = re.sub(r'\\bis at most\\b', '<=', expr)\n"
"        expr = re.sub(r'\\bis greater than or equal to\\b', '>=', expr)\n"
"        expr = re.sub(r'\\bis less than or equal to\\b', '<=', expr)\n"
"        expr = re.sub(r'\\bis greater than\\b', '>', expr)\n"
"        expr = re.sub(r'\\bis less than\\b', '<', expr)\n"
"        expr = re.sub(r'\\bgreater than\\b', '>', expr)\n"
"        expr = re.sub(r'\\bless than\\b', '<', expr)\n"
"        expr = re.sub(r'\\bequal to\\b', '==', expr)\n"
"        expr = re.sub(r'\\bequals\\b', '==', expr)\n"
"        expr = re.sub(r'\\bmultiplied by\\b', '*', expr)\n"
"        expr = re.sub(r'\\bdivided by\\b', '/', expr)\n"
"        expr = re.sub(r'\\bplus\\b', '+', expr)\n"
"        expr = re.sub(r'\\bminus\\b', '-', expr)\n"
"        expr = re.sub(r'\\b(mod|modulo|modulus|modulous|modoulous)\\b', '%', expr)\n"
"        expr = re.sub(r'\\b(?:ask\\s+user\\s+with|ask\\s+with|ask)\\s+\"([^\"]*)\"', r'_smart_input(\"\\1\")', expr)\n"
"        expr = re.sub(r'\\b(?:input\\s+with|input)\\s+\"([^\"]*)\"', r'_smart_input(\"\\1\")', expr)\n"
"        expr = re.sub(r'\\bread\\s+from\\s+user\\s+with\\s+\"([^\"]*)\"', r'_smart_input(\"\\1\")', expr)\n"
"        expr = re.sub(r'\\b(?:ask\\s+user|read\\s+from\\s+user|read\\s+input)\\b', r'_smart_input()', expr)\n"
"        expr = re.sub(r'\\binput\\s*\\((.*?)\\)', r'_smart_input(\\1)', expr)\n"
"        expr = re.sub(r'\\binput\\b(?!\\s*\\(|\\s*_)', r'_smart_input()', expr)\n"
"        expr = re.sub(r'\\bcall\\s+([a-zA-Z0-9_]+)\\s+with\\s+(.*?)\\s+from\\s+\"([^\"]+)\"', r'\\3.\\1(\\2)', expr)\n"
"        expr = re.sub(r'\\bcall\\s+([a-zA-Z0-9_]+)\\s+from\\s+\"([^\"]+)\"', r'\\2.\\1()', expr)\n"
"        expr = re.sub(r'\\bcall\\s+([a-zA-Z0-9_]+)\\s+with\\s+(.*?)(?=[,\\):]|$)', r'\\1(\\2)', expr)\n"
"        expr = re.sub(r'\\bcall\\s+([a-zA-Z0-9_]+)\\b', r'\\1()', expr)\n"
"        expr = re.sub(r'\\bcount of\\s+([a-zA-Z0-9_\\[\\]\"\\']+)', r'len(\\1)', expr)\n"
"        expr = re.sub(r'(\"[^\"]*\")\\s*\\+\\s*([a-zA-Z0-9_\\[\\]]+)', r'\\1 + str(\\2)', expr)\n"
"        expr = re.sub(r'([a-zA-Z0-9_\\[\\]]+)\\s*\\+\\s*(\"[^\"]*\")', r'str(\\1) + \\2', expr)\n"
"        return expr\n"
"    m = re.match(r'^(?:ask\\s+user\\s+for|read)\\s+([a-zA-Z0-9_]+)(?:\\s+from\\s+user)?(?:\\s+with\\s+\"([^\"]*)\")?$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = _smart_input(\"{m.group(2) or \"\"}\")'\n"
"    m = re.match(r'^(?:create\\s+(?:a\\s+|an\\s+|the\\s+)?|declare\\s+|let\\s+)([a-zA-Z0-9_]+)\\s+from\\s+(?:user\\s+)?input(?:\\s+with\\s+\"([^\"]*)\")?$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = _smart_input(\"{m.group(2) or \"\"}\")'\n"
"    m = re.match(r'^(?:create\\s+(?:a\\s+|an\\s+|the\\s+)?|declare\\s+|initialize\\s+(?:the\\s+)?|let\\s+)([a-zA-Z0-9_]+)\\s+(?:of|as|to|=)\\s+(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = {fix_expr(m.group(2))}'\n"
"    m = re.match(r'^set\\s+([a-zA-Z0-9_\\[\\]\"\\.]+)\\s+(?:to|=)\\s+(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = {fix_expr(m.group(2))}'\n"
"    m = re.match(r'^set\\s+([a-zA-Z0-9_\\[\\]\"\\.]+)\\s*(\\+=|-=|\\*=|/=|%=)\\s*(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} {m.group(2)} {fix_expr(m.group(3))}'\n"
"    m = re.match(r'^(?:display|show|output|print)\\s+(.*)$', trimmed)\n"
"    if m: return f'{indent}print({fix_expr(m.group(1))})'\n"
"    m = re.match(r'^(?:while|repeat\\s+while)\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}while {fix_expr(m.group(1))}:'\n"
"    m = re.match(r'^if\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}if {fix_expr(m.group(1))}:'\n"
"    m = re.match(r'^(?:else\\s+if|elif)\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}elif {fix_expr(m.group(1))}:'\n"
"    if trimmed == 'else:': return f'{indent}else:'\n"
"    m = re.match(r'^for\\s+(?:each|every|all)\\s+([a-zA-Z0-9_]+)\\s+in\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}for {m.group(1)} in {fix_expr(m.group(2))}:'\n"
"    m = re.match(r'^for\\s+([a-zA-Z0-9_]+)\\s+from\\s+(.*?)\\s+to\\s+(.*?)(?:\\s+by\\s+(.*?))?:$', trimmed)\n"
"    if m:\n"
"        v, s, e, st = m.group(1), m.group(2), m.group(3), m.group(4)\n"
"        if st: return f'{indent}for {v} in range({fix_expr(s)}, ({fix_expr(e)}) + 1, {fix_expr(st)}):'\n"
"        return f'{indent}for {v} in range({fix_expr(s)}, ({fix_expr(e)}) + 1):'\n"
"    m = re.match(r'^(?:define\\s+function|function)\\s+([a-zA-Z0-9_]+)(?:\\s+with\\s+(.*?))?:$', trimmed)\n"
"    if m: return f'{indent}def {m.group(1)}({m.group(2) or \"\"}):'\n"
"    m = re.match(r'^use\\s+library\\s+\"([^\"]+)\"', trimmed)\n"
"    if m: return f'{indent}# [MODULE: {m.group(1)}]'\n"
"    return f'{indent}{fix_expr(trimmed)}'\n"
"\n"
"if len(sys.argv) > 1:\n"
"    src_file = sys.argv[1]\n"
"    if os.path.exists(src_file):\n"
"        with open(src_file, 'r', encoding='utf-8', errors='replace') as f:\n"
"            content = f.read()\n"
"        transpiled = '\\n'.join(transpile_line(l) for l in content.splitlines())\n"
"        exec(compile(transpiled, src_file, 'exec'))\n"
"    else:\n"
"        print(f'[ENLNG ERROR] File not found: {src_file}', file=sys.stderr)\n"
"        sys.exit(1)\n"
;

void print_help() {
    printf("Enlng Sovereign Programming Language Compiler & Runtime v%s\n", ENLNG_VERSION);
    printf("Dedicated compiler for domain: 'enlng' (pure core language)\n\n");
    printf("Usage:\n");
    printf("  enlng run <file.enlng>    Execute a sovereign enlng script\n");
    printf("  enlng <file.enlng>        Direct script execution\n");
    printf("  enlng --version           Display compiler version\n");
}

int run_enlng_file(const char* filepath) {
    char temp_script[MAX_PATH];
    char temp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_dir);
    snprintf(temp_script, sizeof(temp_script), "%senlng_core_%lu.py", temp_dir, GetCurrentProcessId());

    FILE* f = fopen(temp_script, "w");
    if (!f) {
        fprintf(stderr, "[ENLNG ERROR] Could not create execution sandbox.\n");
        return 1;
    }
    fputs(CORE_ENLNG_RUNNER, f);
    fclose(f);

    char cmd[MAX_PATH * 4];
    snprintf(cmd, sizeof(cmd), "python -u -q \"%s\" \"%s\"", temp_script, filepath);
    int ret = system(cmd);
    remove(temp_script);
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlng version %s (Sovereign Core)\n", ENLNG_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] Usage: enlng run <file.enlng>\n");
            return 1;
        }
        return run_enlng_file(argv[2]);
    }

    return run_enlng_file(argv[1]);
}
