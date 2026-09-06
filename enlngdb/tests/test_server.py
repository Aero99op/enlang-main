"""Exhaustive Integration Tests for enlngdb HTTP Server Daemon (ZERO EXTERNAL DEPENDENCIES)."""

import time
import json
import socket
import urllib.request
import urllib.error
import threading
import pytest

from enlngdb.server import serve_enlngdb, EnlngDBHttpServer


def find_free_port() -> int:
    """Finds a free TCP port on localhost."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


@pytest.fixture(scope="module")
def live_server():
    """Spins up a live EnlngDB HTTP Server Daemon in a background thread."""
    port = find_free_port()
    server = serve_enlngdb(db_path=None, host="127.0.0.1", port=port)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    time.sleep(0.1)  # Brief warm-up

    base_url = f"http://127.0.0.1:{port}"
    yield base_url

    server.shutdown()
    server.server_close()


def test_server_health_check(live_server):
    """Verifies GET /api/health returns healthy status and engine metadata."""
    url = f"{live_server}/api/health"
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req) as resp:
        assert resp.status == 200
        data = json.loads(resp.read().decode("utf-8"))
        assert data["status"] == "healthy"
        assert "EnlngDB" in data["engine"]
        assert "uptime_seconds" in data


def test_server_query_crud_lifecycle(live_server):
    """Executes full DDL, INSERT, and SELECT queries through POST /api/query."""
    url = f"{live_server}/api/query"

    # 1. Create table
    create_payload = json.dumps({
        "query": 'create table scholars with id, name, cgpa;'
    }).encode("utf-8")
    req = urllib.request.Request(url, data=create_payload, headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req) as resp:
        data = json.loads(resp.read().decode("utf-8"))
        assert data["success"] is True

    # 2. Insert rows
    insert_queries = [
        'insert record into scholars with id 1, name "Aryan Sharma", cgpa 9.4;',
        'insert record into scholars with id 2, name "Meera Sen", cgpa 9.8;',
        'insert record into scholars with id 3, name "Dev Patel", cgpa 8.9;'
    ]
    for q in insert_queries:
        req = urllib.request.Request(url, data=json.dumps({"query": q}).encode("utf-8"), headers={"Content-Type": "application/json"}, method="POST")
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            assert data["success"] is True

    # 3. Query with filter
    find_payload = json.dumps({
        "query": 'find records from scholars where cgpa is greater than 9.0;'
    }).encode("utf-8")
    req = urllib.request.Request(url, data=find_payload, headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req) as resp:
        assert resp.status == 200
        data = json.loads(resp.read().decode("utf-8"))
        assert data["success"] is True
        assert data["count"] == 2
        names = [r["name"] for r in data["rows"]]
        assert "Aryan Sharma" in names
        assert "Meera Sen" in names
        assert "Dev Patel" not in names


def test_server_batch_execution(live_server):
    """Executes a batch of queries in a single request through POST /api/batch."""
    url = f"{live_server}/api/batch"
    batch_payload = json.dumps({
        "queries": [
            'create table inventory with item_id, title, stock;',
            'insert record into inventory with item_id 101, title "Microchip A1", stock 50;',
            'insert record into inventory with item_id 102, title "Quantum Core", stock 12;'
        ]
    }).encode("utf-8")

    req = urllib.request.Request(url, data=batch_payload, headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req) as resp:
        assert resp.status == 200
        data = json.loads(resp.read().decode("utf-8"))
        assert data["success"] is True
        assert data["executed_queries"] == 3


def test_server_cors_headers(live_server):
    """Verifies that CORS preflight and responses allow web app access."""
    url = f"{live_server}/api/health"
    req = urllib.request.Request(url, method="OPTIONS")
    with urllib.request.urlopen(req) as resp:
        assert resp.status == 204
        assert resp.headers.get("Access-Control-Allow-Origin") == "*"
        assert "POST" in resp.headers.get("Access-Control-Allow-Methods")
