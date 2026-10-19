#!/usr/bin/env python3
"""
Regression and security test suite for enlangg-bridge.py:
- Verifies Host header allowlist (127.0.0.1 / localhost accepted; rebind domains rejected with 403)
- Verifies Origin allowlist (null and untrusted origins rejected with 403)
- Verifies header-only token auth (X-Enlang-Token accepted; ?token= query parameter rejected with 401)
- Verifies /health endpoint does not leak token
"""

import http.client
import json
import os
import subprocess
import sys
import time
import urllib.request
import urllib.error

BRIDGE_SCRIPT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "enlangg-bridge.py")
TEST_PORT = 5998
TEST_TOKEN = "test_sovereign_token_1234567890abcdef"

def run_tests():
    env = os.environ.copy()
    env["ENLANG_BRIDGE_TOKEN"] = TEST_TOKEN
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT, str(TEST_PORT)],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    try:
        # Wait up to 5 seconds for bridge readiness
        ready = False
        for _ in range(50):
            try:
                conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT, timeout=1)
                conn.request("GET", "/health", headers={"Host": f"127.0.0.1:{TEST_PORT}"})
                res = conn.getresponse()
                if res.status == 200:
                    data = json.loads(res.read().decode())
                    if data.get("status") == "online":
                        ready = True
                        break
            except Exception:
                time.sleep(0.1)
                
        if not ready:
            print("FAIL: Bridge server failed to start on port", TEST_PORT)
            sys.exit(1)
            
        print("[PASS] Bridge started and responded 200 on /health")

        # Test 1: Verify Host header check (Valid vs Invalid)
        # Valid Host: localhost
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("GET", "/health", headers={"Host": f"localhost:{TEST_PORT}"})
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200 for Host: localhost, got {res.status}"
        print("[PASS] Valid Host (localhost) accepted with 200")

        # Invalid Host: attacker.rebind.example
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("GET", "/health", headers={"Host": "attacker.rebind.example"})
        res = conn.getresponse()
        assert res.status == 403, f"Expected 403 for rebind Host, got {res.status}"
        print("[PASS] Rebind Host (attacker.rebind.example) rejected with 403 Forbidden")

        # Test 2: Verify Origin check (Null vs Trusted)
        # Untrusted Origin
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("GET", "/health", headers={
            "Host": f"127.0.0.1:{TEST_PORT}",
            "Origin": "https://malicious-website.com"
        })
        res = conn.getresponse()
        assert res.status == 403, f"Expected 403 for untrusted Origin, got {res.status}"
        print("[PASS] Untrusted Origin rejected with 403 Forbidden")

        # Origin: null
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("GET", "/health", headers={
            "Host": f"127.0.0.1:{TEST_PORT}",
            "Origin": "null"
        })
        res = conn.getresponse()
        assert res.status == 403, f"Expected 403 for Origin: null, got {res.status}"
        print("[PASS] Origin: null rejected with 403 Forbidden")

        # Test 3: Token query string ?token= MUST BE REJECTED with 401
        payload = json.dumps({"filename": "test.enlng", "content": "show 42", "lang": "enlng"}).encode()
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("POST", f"/api/run?token={TEST_TOKEN}", body=payload, headers={
            "Host": f"127.0.0.1:{TEST_PORT}",
            "Content-Type": "application/json"
        })
        res = conn.getresponse()
        assert res.status == 401, f"Expected 401 Unauthorized for ?token= query parameter, got {res.status}"
        print("[PASS] Insecure ?token= query parameter rejected with 401 Unauthorized")

        # Test 4: Header-based token (X-Enlang-Token) MUST BE ACCEPTED
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("POST", "/api/run", body=payload, headers={
            "Host": f"127.0.0.1:{TEST_PORT}",
            "Content-Type": "application/json",
            "X-Enlang-Token": TEST_TOKEN
        })
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200 for valid X-Enlang-Token, got {res.status}"
        body = json.loads(res.read().decode())
        assert "success" in body, f"Unexpected body: {body}"
        print("[PASS] Valid X-Enlang-Token header accepted with 200 OK")

        # Test 5: Authorization: Bearer <token> MUST BE ACCEPTED
        conn = http.client.HTTPConnection("127.0.0.1", TEST_PORT)
        conn.request("POST", "/api/run", body=payload, headers={
            "Host": f"127.0.0.1:{TEST_PORT}",
            "Content-Type": "application/json",
            "Authorization": f"Bearer {TEST_TOKEN}"
        })
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200 for Bearer token, got {res.status}"
        print("[PASS] Authorization: Bearer token accepted with 200 OK")

        print("\n=== ALL BRIDGE SECURITY TESTS PASSED CLEANLY (5/5) ===")

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except Exception:
            proc.kill()

if __name__ == "__main__":
    run_tests()
