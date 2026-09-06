"""Exhaustive Tests for Memory Paging & Buffer Pool in enlngdb (ZERO EXTERNAL DEPENDENCIES)."""

from enlngdb.paging import LRUPageCache, PagedRecordStream, estimate_memory_bytes
from enlngdb.storage import NativeStorageEngine


def test_lru_page_cache_eviction():
    """Verifies that LRUPageCache evicts oldest pages and calls on_evict for dirty pages."""
    evicted_pages = {}

    def handle_evict(page_id, data):
        evicted_pages[page_id] = data

    cache = LRUPageCache(capacity=3, on_evict=handle_evict)

    # Insert 3 pages
    cache.put(1, [{"id": 1}], is_dirty=True)
    cache.put(2, [{"id": 2}], is_dirty=False)
    cache.put(3, [{"id": 3}], is_dirty=True)

    assert len(cache) == 3

    # Access page 1 so it becomes recently used
    page1 = cache.get(1)
    assert page1 == [{"id": 1}]

    # Now put page 4 -> should evict page 2 (least recently used, since page 1 was accessed via get)
    cache.put(4, [{"id": 4}])
    assert len(cache) == 3
    assert 2 not in cache._cache
    assert 1 in cache._cache
    assert 3 in cache._cache
    assert 4 in cache._cache

    # Page 2 was not dirty, so not in evicted_pages
    assert 2 not in evicted_pages

    # Put page 5 -> should evict page 3 (least recently used of [3, 1, 4], marked dirty)
    cache.put(5, [{"id": 5}])
    assert 3 in evicted_pages
    assert evicted_pages[3] == [{"id": 3}]


def test_paged_record_stream_limit_and_offset():
    """Verifies zero-copy generator streaming with offset and limit."""
    def record_generator():
        for i in range(100):
            yield {"id": i, "val": f"item_{i}"}

    # Stream with offset 10 and limit 5
    stream = PagedRecordStream.stream_filter(
        record_generator(),
        filter_fn=lambda r: r["id"] % 2 == 0,  # even numbers: 0, 2, 4, 6...
        offset=5,   # skip first 5 even numbers (0, 2, 4, 6, 8) -> starts at 10
        limit=3     # take 3 even numbers (10, 12, 14)
    )

    results = list(stream)
    assert len(results) == 3
    assert [r["id"] for r in results] == [10, 12, 14]


def test_native_storage_find_limit_and_offset():
    """Verifies limit and offset on NativeStorageEngine."""
    engine = NativeStorageEngine()
    engine.create_table("ranks")
    for i in range(1, 11):
        engine.insert("ranks", {"rank": i, "title": f"Player_{i}"})

    # Find with offset 3 and limit 4
    results = engine.find("ranks", limit=4, offset=3)
    assert len(results) == 4
    assert [r["rank"] for r in results] == [4, 5, 6, 7]


def test_memory_estimation():
    """Verifies estimate_memory_bytes returns non-zero positive byte count."""
    data = {"name": "Aryan", "scores": [9.4, 9.8, 8.9], "active": True}
    mem = estimate_memory_bytes(data)
    assert mem > 0
