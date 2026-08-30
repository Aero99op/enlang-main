"""enlg Data Standard Library — Pandas / Numpy data tools.

Import this in your .enlg file via: import data
"""


def _try_import(pkg: str):
    import importlib
    try:
        return importlib.import_module(pkg)
    except ImportError:
        return None


pd = _try_import("pandas")
np = _try_import("numpy")

if pd:
    DataFrame = pd.DataFrame
    Series    = pd.Series
    print("[enlg Data] Standard Library loaded — pandas backend ready.")
else:
    class DataFrame:
        def __init__(self, *a, **kw): pass
    class Series:
        def __init__(self, *a, **kw): pass
    print("[enlg Data] Standard Library loaded — stub mode (pandas not installed).")

if np:
    Array     = np.array
    Zeros     = np.zeros
    Ones      = np.ones
