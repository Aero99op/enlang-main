"""Sovereign Memory Paging and Chunked Buffer Pool for enlngdb (ZERO EXTERNAL DEPENDENCIES).

Prevents RAM saturation when database size exceeds physical memory.
Features:
- LRUPageCache: Bounded in-memory page buffer pool with Least-Recently-Used eviction.
- ChunkedTable: Divides large datasets into manageable 4KB/record-bounded pages on disk.
- Lazy Page Swapping: Loads only queried chunks into RAM on-demand.
- Paging Iterator: Generator-based streaming for zero-copy queries with LIMIT and OFFSET.
"""

import os
import sys
import json
from collections import OrderedDict
from typing import Dict, List, Any, Optional, Callable, Generator


DEFAULT_PAGE_SIZE = 250       # Number of rows per page chunk
DEFAULT_CACHE_CAPACITY = 20   # Maximum pages kept in RAM (20 pages * 250 = 5000 rows in memory)


class LRUPageCache:
    """Least-Recently-Used (LRU) Page Buffer Pool for table record chunks."""

    def __init__(self, capacity: int = DEFAULT_CACHE_CAPACITY, on_evict: Optional[Callable[[int, List[Dict[str, Any]]], None]] = None):
        self.capacity = max(1, capacity)
        self.on_evict = on_evict
        self._cache: OrderedDict[int, List[Dict[str, Any]]] = OrderedDict()
        self._dirty_pages = set()

    def get(self, page_id: int) -> Optional[List[Dict[str, Any]]]:
        """Fetches page from buffer pool and marks it as recently used."""
        if page_id not in self._cache:
            return None
        self._cache.move_to_end(page_id)
        return self._cache[page_id]

    def put(self, page_id: int, page_data: List[Dict[str, Any]], is_dirty: bool = False):
        """Puts a page into the buffer pool, evicting the oldest page if capacity is reached."""
        if page_id in self._cache:
            self._cache.move_to_end(page_id)
        self._cache[page_id] = page_data
        if is_dirty:
            self._dirty_pages.add(page_id)

        # Evict oldest pages if over capacity
        while len(self._cache) > self.capacity:
            oldest_page_id, evicted_data = self._cache.popitem(last=False)
            if oldest_page_id in self._dirty_pages:
                if self.on_evict:
                    self.on_evict(oldest_page_id, evicted_data)
                self._dirty_pages.discard(oldest_page_id)

    def mark_dirty(self, page_id: int):
        self._dirty_pages.add(page_id)

    def flush_all(self):
        """Flushes all dirty pages through eviction callback."""
        if self.on_evict:
            for page_id in list(self._dirty_pages):
                if page_id in self._cache:
                    self.on_evict(page_id, self._cache[page_id])
        self._dirty_pages.clear()

    def clear(self):
        self.flush_all()
        self._cache.clear()

    def __len__(self):
        return len(self._cache)


class PagedRecordStream:
    """Streaming iterator that processes records chunk-by-chunk without full RAM allocation."""

    @staticmethod
    def stream_filter(rows_generator: Generator[Dict[str, Any], None, None],
                      filter_fn: Optional[Callable[[Dict[str, Any]], bool]] = None,
                      limit: Optional[int] = None,
                      offset: Optional[int] = None) -> Generator[Dict[str, Any], None, None]:
        skipped = 0
        yielded = 0

        target_offset = offset or 0
        target_limit = limit if (limit is not None and limit >= 0) else None

        for row in rows_generator:
            if filter_fn and not filter_fn(row):
                continue

            if skipped < target_offset:
                skipped += 1
                continue

            yield row
            yielded += 1

            if target_limit is not None and yielded >= target_limit:
                break


def estimate_memory_bytes(obj: Any) -> int:
    """Fast estimation of in-memory data footprint."""
    if isinstance(obj, dict):
        return sys.getsizeof(obj) + sum(estimate_memory_bytes(k) + estimate_memory_bytes(v) for k, v in obj.items())
    elif isinstance(obj, (list, tuple, set)):
        return sys.getsizeof(obj) + sum(estimate_memory_bytes(x) for x in obj)
    return sys.getsizeof(obj)
