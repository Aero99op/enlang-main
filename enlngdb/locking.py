"""Sovereign Concurrency and Locking Subsystem for enlngdb (ZERO EXTERNAL DEPENDENCIES).

Provides:
- In-process Read-Write Lock (RWLock): multiple concurrent readers, exclusive single writer.
- Cross-process OS FileLock: uses msvcrt on Windows and fcntl on Unix for crash-resilient process locking.
- DatabaseLock: unified lock coordinator managing table and file-level integrity.
"""

import os
import sys
import time
import threading
from contextlib import contextmanager
from typing import Optional


class LockTimeoutError(Exception):
    """Raised when acquiring a lock exceeds the timeout threshold."""
    pass


class RWLock:
    """In-process Re-entrant Read-Write Lock supporting concurrent readers and exclusive writers."""

    def __init__(self):
        self._lock = threading.RLock()
        self._read_ready = threading.Condition(self._lock)
        self._readers = 0
        self._writers_waiting = 0
        self._writer_active = False
        self._writer_thread: Optional[int] = None
        self._writer_depth: int = 0

    def acquire_read(self, timeout: Optional[float] = None):
        """Acquires a shared read lock."""
        start_time = time.time()
        current_thread = threading.get_ident()
        with self._lock:
            # If current thread is already holding write lock, it can read its own writes reentrantly
            if self._writer_thread == current_thread:
                self._readers += 1
                return

            while self._writer_active or self._writers_waiting > 0:
                if timeout is not None:
                    elapsed = time.time() - start_time
                    remaining = timeout - elapsed
                    if remaining <= 0:
                        raise LockTimeoutError("Timed out waiting for read lock.")
                    self._read_ready.wait(remaining)
                else:
                    self._read_ready.wait()
            self._readers += 1

    def release_read(self):
        """Releases a shared read lock."""
        with self._lock:
            self._readers -= 1
            if self._readers == 0:
                self._read_ready.notify_all()

    def acquire_write(self, timeout: Optional[float] = None):
        """Acquires an exclusive write lock (re-entrant for the same thread)."""
        start_time = time.time()
        current_thread = threading.get_ident()
        with self._lock:
            if self._writer_thread == current_thread:
                self._writer_depth += 1
                return

            self._writers_waiting += 1
            try:
                while self._readers > 0 or self._writer_active:
                    if timeout is not None:
                        elapsed = time.time() - start_time
                        remaining = timeout - elapsed
                        if remaining <= 0:
                            raise LockTimeoutError("Timed out waiting for write lock.")
                        self._read_ready.wait(remaining)
                    else:
                        self._read_ready.wait()
                self._writer_active = True
                self._writer_thread = current_thread
                self._writer_depth = 1
            finally:
                self._writers_waiting -= 1

    def release_write(self):
        """Releases an exclusive write lock."""
        with self._lock:
            if self._writer_thread != threading.get_ident():
                return
            self._writer_depth -= 1
            if self._writer_depth == 0:
                self._writer_active = False
                self._writer_thread = None
                self._read_ready.notify_all()

    @contextmanager
    def read_lock(self, timeout: Optional[float] = None):
        self.acquire_read(timeout)
        try:
            yield
        finally:
            self.release_read()

    @contextmanager
    def write_lock(self, timeout: Optional[float] = None):
        self.acquire_write(timeout)
        try:
            yield
        finally:
            self.release_write()


class FileLock:
    """Cross-process OS-level advisory file lock with timeout and exponential backoff."""

    def __init__(self, lock_file_path: str, timeout: float = 10.0):
        self.lock_file_path = lock_file_path
        self.timeout = timeout
        self._fd: Optional[int] = None

    def acquire(self) -> bool:
        """Acquires an exclusive OS-level file lock."""
        start = time.time()
        backoff = 0.005

        os.makedirs(os.path.dirname(os.path.abspath(self.lock_file_path)) or ".", exist_ok=True)

        while True:
            try:
                self._fd = os.open(self.lock_file_path, os.O_CREAT | os.O_RDWR)
                if sys.platform == "win32":
                    import msvcrt
                    # Lock 1 byte from the beginning of the file non-blocking
                    msvcrt.locking(self._fd, msvcrt.LK_NBLCK, 1)
                else:
                    import fcntl
                    fcntl.flock(self._fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                return True
            except (BlockingIOError, PermissionError, OSError):
                if self._fd is not None:
                    try:
                        os.close(self._fd)
                    except OSError:
                        pass
                    self._fd = None

                elapsed = time.time() - start
                if elapsed >= self.timeout:
                    raise LockTimeoutError(
                        f"Failed to acquire cross-process lock on '{self.lock_file_path}' after {self.timeout:.1f}s."
                    )
                time.sleep(backoff)
                backoff = min(backoff * 1.5, 0.1)

    def release(self):
        """Releases the exclusive OS-level file lock."""
        if self._fd is not None:
            try:
                if sys.platform == "win32":
                    import msvcrt
                    try:
                        msvcrt.locking(self._fd, msvcrt.LK_UNLCK, 1)
                    except OSError:
                        pass
                else:
                    import fcntl
                    try:
                        fcntl.flock(self._fd, fcntl.LOCK_UN)
                    except OSError:
                        pass
            finally:
                try:
                    os.close(self._fd)
                except OSError:
                    pass
                self._fd = None

    def __enter__(self):
        self.acquire()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.release()


class DatabaseLock:
    """Unified lock coordinator combining in-process RWLock and cross-process FileLock."""

    def __init__(self, db_path: Optional[str] = None):
        self.db_path = db_path
        self._rwlock = RWLock()
        self._file_lock: Optional[FileLock] = None

        if self.db_path and self.db_path != ":memory:":
            lock_path = f"{self.db_path}.lock"
            self._file_lock = FileLock(lock_path)

    @contextmanager
    def read(self, timeout: Optional[float] = 10.0):
        """Shared read lock across threads."""
        with self._rwlock.read_lock(timeout):
            yield

    @contextmanager
    def write(self, timeout: Optional[float] = 10.0):
        """Exclusive write lock across threads and processes."""
        with self._rwlock.write_lock(timeout):
            if self._file_lock:
                with self._file_lock:
                    yield
            else:
                yield
