
import enlg.runtime.domain_handlers.sec_handlers as _sec
def encrypt(target, *args): return _sec.handle_sec_encrypt(target, list(args))
def scan(target, *args): return _sec.handle_sec_scan(target, list(args))
