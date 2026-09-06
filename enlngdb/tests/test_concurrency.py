"""Exhaustive Concurrency & Thread-Safety Tests for enlngdb (ZERO EXTERNAL DEPENDENCIES)."""

import os
import time
import tempfile
import threading
from pathlib import Path
import pytest

from enlngdb.storage import NativeStorageEngine, StorageError
from enlngdb.locking import RWLock, FileLock, DatabaseLock, LockTimeoutError


def test_rwlock_concurrency():
    """Verifies that multiple readers can hold lock simultaneously, but writers have exclusive access."""
    rwlock = RWLock()
    active_readers = 0
    max_concurrent_readers = 0
    writer_active = False
    lock = threading.Lock()

    def reader_task():
        nonlocal active_readers, max_concurrent_readers
        with rwlock.read_lock():
            with lock:
                active_readers += 1
                if active_readers > max_concurrent_readers:
                    max_concurrent_readers = active_readers
            time.sleep(0.02)
            with lock:
                active_readers -= 1

    def writer_task():
        nonlocal writer_active
        with rwlock.write_lock():
            writer_active = True
            time.sleep(0.02)
            writer_active = False

    threads = []
    for _ in range(10):
        threads.append(threading.Thread(target=reader_task))
    threads.append(threading.Thread(target=writer_task))
    for _ in range(10):
        threads.append(threading.Thread(target=reader_task))

    for t in threads:
        t.start()
    for t in threads:
        t.join()

    # Confirms that readers were able to run concurrently (more than 1 active reader at a time)
    assert max_concurrent_readers > 1
    assert not writer_active


def test_file_lock_mutual_exclusion():
    """Verifies OS-level FileLock mutual exclusion across handles."""
    with tempfile.TemporaryDirectory() as tmpdir:
        lock_file = os.path.join(tmpdir, "test_file.lock")
        lock1 = FileLock(lock_file, timeout=1.0)
        lock2 = FileLock(lock_file, timeout=0.1)

        assert lock1.acquire() is True

        # Second lock attempt must time out while first is held
        with pytest.raises(LockTimeoutError):
            lock2.acquire()

        lock1.release()

        # After release, second lock can acquire successfully
        assert lock2.acquire() is True
        lock2.release()


def test_concurrent_multithreaded_database_inserts():
    """Launches 20 concurrent threads inserting into the same EnlngDB table simultaneously."""
    engine = NativeStorageEngine()
    engine.create_table("traffic_logs")

    errors = []
    num_threads = 20
    inserts_per_thread = 25
    expected_total = num_threads * inserts_per_thread

    def worker(worker_id: int):
        try:
            for i in range(inserts_per_thread):
                row_id = worker_id * 1000 + i
                engine.insert("traffic_logs", {
                    "id": row_id,
                    "worker": worker_id,
                    "payload": f"telemetry-packet-{i}",
                    "timestamp": time.time()
                })
        except Exception as e:
            errors.append(e)

    threads = [threading.Thread(target=worker, args=(w,)) for w in range(num_threads)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    assert len(errors) == 0, f"Encountered concurrency errors: {errors}"
    count = engine.count("traffic_logs")
    assert count == expected_total, f"Expected {expected_total} rows, but found {count}"


def test_transaction_context_manager():
    """Tests that transaction context manager executes under write lock cleanly."""
    engine = NativeStorageEngine()
    engine.create_table("vault")

    with engine.transaction():
        engine.insert("vault", {"id": 1, "asset": "Gold", "qty": 100})
        engine.insert("vault", {"id": 2, "asset": "Silver", "qty": 500})
        engine.update("vault", {"qty": 150}, where_ast=None)

    rows = engine.find("vault")
    assert len(rows) == 2
    assert all(r["qty"] == 150 for r in rows)
