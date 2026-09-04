/*
 * enlng - The Sovereign Universal General-Purpose Programming Language
 * Domain: PURE ENLNG (Complete Python Parity + Natural English Grammar)
 * Zero Web. Zero Mobile. 100% General-Purpose Sovereign Computing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define ENLNG_VERSION "2.0.0-general-purpose-master"

const char* CORE_ENLNG_RUNNER = 
"import sys, os, re, math, time, random, json\n"
"if hasattr(sys.stdout, 'reconfigure'):\n"
"    sys.stdout.reconfigure(encoding='utf-8', errors='replace')\n"
"\n"
"# --- Primitives & Constants ---\n"
"true = True; false = False; null = None; undefined = None\n"
"pi = math.pi; e = math.e\n"
"\n"
"# --- Standard Built-in Utilities ---\n"
"def _smart_input(prompt=''):\n"
"    try: val = input(prompt)\n"
"    except (EOFError, KeyboardInterrupt): return ''\n"
"    try:\n"
"        if '.' in val: return float(val)\n"
"        return int(val)\n"
"    except ValueError: return val\n"
"\n"
"def ask(prompt=''): return _smart_input(prompt)\n"
"def sleep(sec): time.sleep(sec)\n"
"def random_number(a, b): return random.randint(a, b)\n"
"def sqrt(n): return math.sqrt(n)\n"
"def floor(n): return math.floor(n)\n"
"def ceil(n): return math.ceil(n)\n"
"\n"
"# --- Built-in File I/O ---\n"
"def read_file(path):\n"
"    with open(path, 'r', encoding='utf-8', errors='replace') as f: return f.read()\n"
"def write_file(path, content):\n"
"    with open(path, 'w', encoding='utf-8') as f: f.write(str(content))\n"
"def append_file(path, content):\n"
"    with open(path, 'a', encoding='utf-8') as f: f.write(str(content))\n"
"def file_exists(path): return os.path.exists(path)\n"
"\n"
"# --- Test Framework ---\n"
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
"# --- Universal Enlng Compiler Engine ---\n"
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
"        expr = re.sub(r'\\b([a-zA-Z0-9_\\[\\]]+)\\s+contains\\s+(.*)', r'(\\2 in \\1)', expr)\n"
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
"        expr = re.sub(r'\\b(?:count\\s+of|length\\s+of)\\s+([a-zA-Z0-9_\\[\\]\"\\'\\(\\)]+)', r'len(\\1)', expr)\n"
"        return expr\n"
"\n"
"    # 1. Loop controls: break / continue / pass\n"
"    if re.match(r'^(?:break|stop\\s+loop|exit\\s+loop)$', trimmed, re.I): return f'{indent}break'\n"
"    if re.match(r'^(?:continue|skip\\s+iteration|next\\s+iteration)$', trimmed, re.I): return f'{indent}continue'\n"
"    if re.match(r'^(?:pass|do\\s+nothing)$', trimmed, re.I): return f'{indent}pass'\n"
"\n"
"    # 2. Return statements: return / give back\n"
"    m = re.match(r'^(?:return|give\\s+back)(?:\\s+(.*))?$', trimmed, re.I)\n"
"    if m:\n"
"        val = m.group(1)\n"
"        return f'{indent}return {fix_expr(val)}' if val else f'{indent}return'\n"
"\n"
"    # 3. Exception handling: try / catch / finally / raise\n"
"    if re.match(r'^(?:try|attempt):$', trimmed, re.I): return f'{indent}try:'\n"
"    m = re.match(r'^(?:catch|rescue|except)(?:\\s+as\\s+|\\s+)([a-zA-Z0-9_]+):$', trimmed, re.I)\n"
"    if m: return f'{indent}except Exception as {m.group(1)}:'\n"
"    if re.match(r'^(?:catch|rescue|except):$', trimmed, re.I): return f'{indent}except Exception:'\n"
"    if re.match(r'^finally:$', trimmed, re.I): return f'{indent}finally:'\n"
"    m = re.match(r'^(?:raise|throw)\\s+(.*)$', trimmed, re.I)\n"
"    if m: return f'{indent}raise Exception({fix_expr(m.group(1))})'\n"
"\n"
"    # 4. List mutations: add X to Y / remove X from Y\n"
"    m = re.match(r'^add\\s+(.*?)\\s+to\\s+([a-zA-Z0-9_\\[\\]\\.]+)$', trimmed, re.I)\n"
"    if m: return f'{indent}{m.group(2)}.append({fix_expr(m.group(1))})'\n"
"    m = re.match(r'^remove\\s+(.*?)\\s+from\\s+([a-zA-Z0-9_\\[\\]\\.]+)$', trimmed, re.I)\n"
"    if m: return f'{indent}{m.group(2)}.remove({fix_expr(m.group(1))})'\n"
"\n"
"    # 5. OOP: Class & Methods\n"
"    m = re.match(r'^(?:define\\s+class|class)\\s+([a-zA-Z0-9_]+)(?:\\s+(?:inherits\\s+from|extends)\\s+([a-zA-Z0-9_]+))?:$', trimmed, re.I)\n"
"    if m:\n"
"        cname, base = m.group(1), m.group(2)\n"
"        return f'{indent}class {cname}({base}):' if base else f'{indent}class {cname}:'\n"
"    m = re.match(r'^(?:method|define\\s+method)\\s+([a-zA-Z0-9_]+)(?:\\s+with\\s+(.*?))?:$', trimmed, re.I)\n"
"    if m:\n"
"        mname, params = m.group(1), m.group(2) or ''\n"
"        return f'{indent}def {mname}(self, {params}):' if params else f'{indent}def {mname}(self):'\n"
"\n"
"    # 6. Universal Hint Keyword Discovery for 'ask'\n"
"    if re.search(r'\\b(?:ask|asking)\\b', trimmed, re.I) and not re.match(r'^(?:create|declare|initialize|let|set|if|while|for|define|display|show)\\b', trimmed, re.I):\n"
"        _pm = re.search(r'\"([^\"]*)\"', trimmed)\n"
"        _p = _pm.group(1) if _pm else ''\n"
"        _cl = re.sub(r'\"[^\"]*\"', '', trimmed)\n"
"        _cl = re.sub(r'[:=,]', ' ', _cl)\n"
"        _fill = {'ask','asking','the','user','for','to','with','from','a','an','in','into','please','provide','give','enter','input','prompt','take','get'}\n"
"        _vars = [t for t in _cl.split() if t.lower() not in _fill and re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', t)]\n"
"        if _vars: return f'{indent}{_vars[0]} = _smart_input(\"{_p}\")'\n"
"        return f'{indent}_smart_input(\"{_p}\")'\n"
"\n"
"    # 7. Variable Declarations\n"
"    m = re.match(r'^(?:create\\s+(?:a\\s+|an\\s+|the\\s+)?|declare\\s+|initialize\\s+(?:the\\s+)?|let\\s+)([a-zA-Z0-9_]+)\\s+(?:of|as|to|=)\\s+(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = {fix_expr(m.group(2))}'\n"
"\n"
"    # 8. Variable Assignments\n"
"    m = re.match(r'^set\\s+([a-zA-Z0-9_\\[\\]\"\\.]+)\\s+(?:to|=)\\s+(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} = {fix_expr(m.group(2))}'\n"
"    m = re.match(r'^set\\s+([a-zA-Z0-9_\\[\\]\"\\.]+)\\s*(\\+=|-=|\\*=|/=|%=)\\s*(.*)$', trimmed)\n"
"    if m: return f'{indent}{m.group(1)} {m.group(2)} {fix_expr(m.group(3))}'\n"
"\n"
"    # 9. Output Display\n"
"    m = re.match(r'^(?:display|show|output|print)\\s+(.*)$', trimmed)\n"
"    if m:\n"
"        _v = fix_expr(m.group(1))\n"
"        _ch = []; _iq = False; _qc = ''; _bd = 0; _pd = 0\n"
"        for _c in _v:\n"
"            if not _iq and _c in ('\"', \"'\"): _iq = True; _qc = _c; _ch.append(_c)\n"
"            elif _iq and _c == _qc: _iq = False; _ch.append(_c)\n"
"            elif not _iq and _c in ('(', '['): _pd += (_c == '('); _bd += (_c == '['); _ch.append(_c)\n"
"            elif not _iq and _c in (')', ']'): _pd -= (_c == ')'); _bd -= (_c == ']'); _ch.append(_c)\n"
"            elif not _iq and _bd == 0 and _pd == 0 and _c == '+': _ch.append(',')\n"
"            else: _ch.append(_c)\n"
"        return f'{indent}print({\"\".join(_ch)}, sep=\"\")'\n"
"\n"
"    # 10. Loops\n"
"    m = re.match(r'^(?:while|repeat\\s+while)\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}while {fix_expr(m.group(1))}:'\n"
"    m = re.match(r'^for\\s+(?:each|every|all)\\s+([a-zA-Z0-9_]+)\\s+in\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}for {m.group(1)} in {fix_expr(m.group(2))}:'\n"
"    m = re.match(r'^for\\s+([a-zA-Z0-9_]+)\\s+from\\s+(.*?)\\s+to\\s+(.*?)(?:\\s+by\\s+(.*?))?:$', trimmed)\n"
"    if m:\n"
"        v, s, e, st = m.group(1), m.group(2), m.group(3), m.group(4)\n"
"        if st: return f'{indent}for {v} in range({fix_expr(s)}, ({fix_expr(e)}) + 1, {fix_expr(st)}):'\n"
"        return f'{indent}for {v} in range({fix_expr(s)}, ({fix_expr(e)}) + 1):'\n"
"\n"
"    # 11. Conditionals\n"
"    m = re.match(r'^if\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}if {fix_expr(m.group(1))}:'\n"
"    m = re.match(r'^(?:else\\s+if|elif)\\s+(.*?):$', trimmed)\n"
"    if m: return f'{indent}elif {fix_expr(m.group(1))}:'\n"
"    if trimmed == 'else:': return f'{indent}else:'\n"
"\n"
"    # 12. Functions\n"
"    m = re.match(r'^(?:define\\s+function|function|def)\\s+([a-zA-Z0-9_]+)(?:\\s+with\\s+(.*?))?:$', trimmed)\n"
"    if m: return f'{indent}def {m.group(1)}({m.group(2) or \"\"}):'\n"
"\n"
"    # 13. Modules & Libraries\n"
"    m = re.match(r'^(?:use\\s+library|import)\\s+\"?([a-zA-Z0-9_]+)\"?', trimmed)\n"
"    if m: return f'{indent}import {m.group(1)}'\n"
"\n"
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
    printf("Full General-Purpose Language (Complete Python Parity + Natural English Grammar)\n\n");
    printf("Usage:\n");
    printf("  enlng run <file.enlng>    Compile and execute enlng script\n");
    printf("  enlng <file.enlng>        Direct execution\n");
    printf("  enlng --version           Show version\n");
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
        printf("enlng version %s (General Purpose)\n", ENLNG_VERSION);
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
