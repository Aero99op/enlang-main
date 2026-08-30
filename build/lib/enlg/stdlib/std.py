# enlg/stdlib/std.py
# The Master "Dot-Free" Wrapper Library for enlg

import builtins
import os
import sys

# Try importing common libraries for fallback
try: import pandas as pd
except ImportError: pd = None

try: import numpy as np
except ImportError: np = None

try: import math
except ImportError: math = None

# ── Data Science Wrappers ──────────────────────────────────────────────
def read_csv(filepath):
    if pd: return pd.read_csv(filepath)
    raise RuntimeError("pandas not installed")

def read_json(filepath):
    if pd: return pd.read_json(filepath)
    raise RuntimeError("pandas not installed")

def drop_empty(df):
    if hasattr(df, "dropna"): return df.dropna()
    return df

def copy_data(df):
    if hasattr(df, "copy"): return df.copy()
    return df

def slice_data(df, start, end):
    start = int(start) if isinstance(start, str) else start
    end = int(end) if isinstance(end, str) else end
    if hasattr(df, "iloc"): return df.iloc[start:end]
    return df[start:end]

# ── NumPy & Math Wrappers ──────────────────────────────────────────────
def array(data):
    if np:
        cleaned = []
        for x in data:
            if isinstance(x, str):
                try:
                    num = float(x)
                    x = int(num) if num.is_integer() else num
                except ValueError:
                    pass
            cleaned.append(x)
        return np.array(cleaned)
    raise RuntimeError("numpy not installed")

def mean(data):
    if np: return np.mean(data)
    raise RuntimeError("numpy not installed")

def zeros(shape):
    if np: return np.zeros(shape)
    raise RuntimeError("numpy not installed")

def ones(shape):
    if np: return np.ones(shape)
    raise RuntimeError("numpy not installed")

def reshape(arr, shape):
    if np: return np.reshape(arr, shape)
    raise RuntimeError("numpy not installed")

def sum_all(data):
    if np: return np.sum(data)
    return sum(data)

def matrix_mul(a, b):
    if np: return np.matmul(a, b)
    raise RuntimeError("numpy not installed")

def get_length(obj):
    return len(obj)

def get_item(obj, index):
    index = int(index) if isinstance(index, str) else index
    return obj[index]

def print_type(obj):
    return type(obj).__name__

def read_text(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_text(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(str(content))
        
def get_env(key):
    return os.environ.get(key, "")

def list_files(dirpath="."):
    return os.listdir(dirpath)

# ── The "God Call" (Run any python method using spaces instead of dots) ─
def run_py(path_string, *args):
    """
    Examples:
    run_py("pandas read_csv", "file.csv") -> pandas.read_csv("file.csv")
    run_py("math sqrt", 16) -> math.sqrt(16)
    """
    parts = path_string.split(" ")
    
    import importlib
    try:
        obj = importlib.import_module(parts[0])
    except ImportError:
        if hasattr(builtins, parts[0]):
            obj = getattr(builtins, parts[0])
        else:
            raise RuntimeError(f"Could not find module or object '{parts[0]}'")
            
    for p in parts[1:]:
        obj = getattr(obj, p)
        
    if callable(obj):
        return obj(*args)
    return obj

