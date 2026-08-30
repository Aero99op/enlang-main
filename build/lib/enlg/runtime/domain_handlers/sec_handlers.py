"""enlg Cybersecurity Handler Module.

Handler signature: handler(target, op_args: list, env=None) -> result
"""
import hashlib


def handle_sec_scan(target, op_args: list, env=None):
    """scan target on port 80"""
    port = op_args[0] if op_args else 80
    try:
        port = int(port)
    except (ValueError, TypeError):
        port = 80
    print(f"[enlg CyberSec] Scanning '{target}' on port {port}...")
    return {"target": str(target), "port": port, "status": "OPEN"}


def handle_sec_encrypt(target, op_args: list, env=None):
    """encrypt payload using sha256"""
    algo = str(op_args[0]).lower() if op_args else "sha256"
    data = str(target).encode("utf-8")
    if algo == "sha256":
        res = hashlib.sha256(data).hexdigest()
    elif algo == "md5":
        res = hashlib.md5(data).hexdigest()
    elif algo == "sha512":
        res = hashlib.sha512(data).hexdigest()
    else:
        res = hashlib.sha256(data).hexdigest()
    print(f"[enlg CyberSec] Encrypted with {algo.upper()}: {res[:16]}...")
    return res


SEC_HANDLERS: dict = {
    "SEC_SCAN":    handle_sec_scan,
    "SEC_ENCRYPT": handle_sec_encrypt,
}
