"""Python API Usage Examples for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

import os
import sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from enlngdb import run_enlngdb_source, NativeStorageEngine, NativeExecutionEngine


def example_1_run_conversational_source():
    """Example 1: Running conversational English database queries from a Python string."""
    print("--- [EXAMPLE 1] Running Conversational English Source Code ---")
    script = """
    type enlngdb
    create table if not exists books with id, title, author, price, rating
    insert record into books with id 1, title "Clean Architecture", author "Uncle Bob", price 40, rating 4.8
    insert record into books with id 2, title "Designing Data-Intensive Apps", author "Martin Kleppmann", price 55, rating 4.9
    insert record into books with id 3, title "The Pragmatic Programmer", author "Hunt & Thomas", price 45, rating 4.7

    display ">> Searching for top-rated computer science books (rating >= 4.8):"
    find records from books where rating is at least 4.8 order by price descending
    """
    reports = run_enlngdb_source(script, stream_output=True)
    print(f"Executed {len(reports)} statements successfully!\n")


def example_2_pure_python_programmatic_storage():
    """Example 2: Using the Native Storage Engine programmatically in pure Python (Zero SQL, Zero SQLite)."""
    print("--- [EXAMPLE 2] Programmatic Native Storage Engine (Pure Python API) ---")
    store = NativeStorageEngine()

    # 1. Create a table
    store.create_table("servers", hints={"tier": "edge"})

    # 2. Insert records
    store.insert("servers", {"ip": "192.168.1.10", "region": "ap-south-1", "load": 0.42, "status": "healthy"})
    store.insert("servers", {"ip": "192.168.1.11", "region": "us-east-1", "load": 0.89, "status": "overloaded"})
    store.insert("servers", {"ip": "192.168.1.12", "region": "eu-central-1", "load": 0.15, "status": "healthy"})

    # 3. Query records
    healthy_servers = store.find("servers")
    print(f"Total servers in memory: {len(healthy_servers)}")
    for s in healthy_servers:
        print(f"  - Server {s['ip']} [{s['region']}]: status={s['status']}, load={s['load']}")

    # 4. Fast Count & Update
    count = store.count("servers")
    print(f"\nTotal Server Count: {count}")

    # 5. Persist to disk as sovereign .edb binary file
    edb_filename = "servers_cluster.edb"
    store.save_to_disk(edb_filename)
    print(f"Saved cluster snapshot to sovereign file: {edb_filename}")

    # 6. Reload from disk
    restored_store = NativeStorageEngine(db_path=edb_filename)
    print(f"Restored {len(restored_store.find('servers'))} servers from '{edb_filename}'.")

    # Cleanup temporary demo file
    if os.path.exists(edb_filename):
        os.remove(edb_filename)
    print("\n--- Example completed cleanly! ---")


if __name__ == "__main__":
    example_1_run_conversational_source()
    example_2_pure_python_programmatic_storage()
