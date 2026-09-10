import os
import sys
import time
import subprocess
import glob

SUITE_DIR = os.path.abspath("tests/suite100")
ENLNG_EXE = os.path.abspath("enlng.exe")

def main():
    if not os.path.exists(ENLNG_EXE):
        print(f"Error: {ENLNG_EXE} not found!")
        sys.exit(1)

    test_files = sorted(glob.glob(os.path.join(SUITE_DIR, "test_*.enlng")))
    total = len(test_files)
    print(f"======================================================================")
    print(f"  ENLANG SOVEREIGN ENGINE - MASTER 100 TESTS EXECUTION HARNESS")
    print(f"  Testing {total} Complex & Comprehensive Features Across All Domains")
    print(f"======================================================================\n")

    passed = 0
    failed = 0
    failures = []

    start_total = time.time()

    for idx, test_path in enumerate(test_files, 1):
        test_name = os.path.basename(test_path)
        t0 = time.time()
        
        try:
            res = subprocess.run(
                [ENLNG_EXE, "run", test_path],
                capture_output=True,
                text=True,
                timeout=15
            )
            elapsed = int((time.time() - t0) * 1000)
            
            output = res.stdout.strip()
            if res.returncode == 0 and "[TEST PASS" in output:
                passed += 1
                # Extract pass label
                pass_label = [line for line in output.splitlines() if "[TEST PASS" in line]
                summary_label = pass_label[-1] if pass_label else output
                print(f"[{idx:03d}/{total}] {test_name:<42} -> OK ({elapsed}ms) | {summary_label}")
            else:
                failed += 1
                err_msg = res.stderr.strip() if res.stderr else output
                failures.append((test_name, res.returncode, err_msg))
                print(f"[{idx:03d}/{total}] {test_name:<42} -> FAILED (code {res.returncode}) ({elapsed}ms)")
                print(f"       Details: {err_msg[:200]}")
        except subprocess.TimeoutExpired:
            failed += 1
            failures.append((test_name, -1, "TIMEOUT after 15s"))
            print(f"[{idx:03d}/{total}] {test_name:<42} -> TIMEOUT")

    total_time = round(time.time() - start_total, 2)

    # Cleanup temporary test files
    for tmp in ["test_091_tmp.txt", "test_092_tmp.txt", "test_093_tmp.txt", "test_094_tmp.csv", "test_100_capstone.txt"]:
        if os.path.exists(tmp):
            try: os.remove(tmp)
            except: pass

    print(f"\n======================================================================")
    print(f"  FINAL HARNESS RESULTS:")
    print(f"  Total Tests Executed: {total}")
    print(f"  Passed: {passed} / {total} ({(passed/total)*100:.1f}%)")
    print(f"  Failed: {failed}")
    print(f"  Total Elapsed Time: {total_time}s")
    print(f"======================================================================")

    if failed > 0:
        print("\nFailures Summary:")
        for name, code, msg in failures:
            print(f"  - {name} (Exit Code: {code}): {msg[:100]}")
        sys.exit(1)
    else:
        print("\n>>> ALL 100 TESTS PASSED FLAWLESSLY! ENLANG SOVEREIGN CORE IS 100% BULLETPROOF! <<<")
        sys.exit(0)

if __name__ == "__main__":
    main()
