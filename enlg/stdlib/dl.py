"""enlg DL Standard Library — Deep Learning (PyTorch).

Import this in your .enlg file via: import dl
"""


def _try_import(pkg: str):
    import importlib
    try:
        return importlib.import_module(pkg)
    except ImportError:
        return None


torch = _try_import("torch")
nn = _try_import("torch.nn")

if torch and nn:
    NeuralNet = nn.Sequential
    Dense     = nn.Linear
    ReLU      = nn.ReLU
    Sigmoid   = nn.Sigmoid
    Softmax   = nn.Softmax
    Dropout   = nn.Dropout
    print("[enlg DL] Standard Library loaded — PyTorch backend ready.")
else:
    # Lightweight stubs so enlg code doesn't crash without torch
    class _Stub:
        def __init__(self, *a, **kw): pass
        def __repr__(self): return f"<DL Stub: torch not installed>"

    NeuralNet = _Stub
    Dense     = _Stub
    ReLU      = _Stub
    Sigmoid   = _Stub
    Softmax   = _Stub
    Dropout   = _Stub
    print("[enlg DL] Standard Library loaded — stub mode (torch not installed).")

import enlg.runtime.domain_handlers.dl_handlers as _dl
def compile(target, *args): return _dl.handle_dl_compile(target, list(args))
