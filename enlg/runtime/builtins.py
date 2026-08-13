"""Standard Library Built-ins.

Provides native Python callables that are injected into the 
global runtime environment of the enlg execution engine.
"""

from typing import Any, Callable, Dict

def native_length(obj: Any) -> int:
    try:
        return len(obj)
    except TypeError:
        return 0

def native_type_of(obj: Any) -> str:
    return type(obj).__name__

def native_to_string(obj: Any) -> str:
    return str(obj)

def native_to_number(obj: Any) -> float:
    try:
        return float(obj)
    except (ValueError, TypeError):
        return 0.0

def native_print_range(start: Any, end: Any) -> None:
    try:
        s = int(start)
        e = int(end)
        for i in range(s, e + 1):
            print(i)
    except (ValueError, TypeError):
        pass

# The built-in registry
BUILTINS: Dict[str, Callable] = {
    "length": native_length,
    "type_of": native_type_of,
    "to_string": native_to_string,
    "to_number": native_to_number,
    "print_range": native_print_range,
    "range": native_print_range,
}
