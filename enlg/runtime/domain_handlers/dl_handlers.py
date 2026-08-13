"""enlg Deep Learning Handler Module — PyTorch backed.

Handles DL_COMPILE, DL_FIT, DL_FORWARD.
If torch is not installed, falls back gracefully with warnings.

Handler signature: handler(target, op_args: list, env=None) -> result
"""


def _try_import(pkg: str):
    import importlib
    try:
        return importlib.import_module(pkg)
    except ImportError:
        print(f"[enlg DL] Warning: '{pkg}' not installed. Run: pip install {pkg.split('.')[0]}")
        return None


# ─── DL_COMPILE ──────────────────────────────────────────────────────────────

def handle_dl_compile(target, op_args: list, env=None):
    """
    compile net with layers=[Dense 784 128, ReLU, Dense 128 10]
    
    target  = model object (torch.nn.Module or enlg class instance)
    op_args = [layer_spec_list]
    Returns compiled model.
    """
    torch = _try_import("torch")
    nn = _try_import("torch.nn")

    if torch and nn:
        layers = []
        layer_spec = op_args[0] if op_args else []
        if isinstance(layer_spec, list):
            for spec in layer_spec:
                s = str(spec).strip()
                if s.lower().startswith("dense"):
                    parts = s.split()
                    in_f = int(parts[1]) if len(parts) > 1 else 128
                    out_f = int(parts[2]) if len(parts) > 2 else 64
                    layers.append(nn.Linear(in_f, out_f))
                elif s.lower() == "relu":
                    layers.append(nn.ReLU())
                elif s.lower() == "sigmoid":
                    layers.append(nn.Sigmoid())
                elif s.lower() == "softmax":
                    layers.append(nn.Softmax(dim=1))
                elif s.lower() == "dropout":
                    layers.append(nn.Dropout(0.3))
        model = nn.Sequential(*layers)
        print(f"[enlg DL] Compiled neural network: {model}")
        return model

    # Fallback stub
    print(f"[enlg DL] Compiled model stub (torch not available).")
    return {"type": "NeuralNet", "compiled": True, "layers": op_args}


# ─── DL_FIT ──────────────────────────────────────────────────────────────────

def handle_dl_fit(target, op_args: list, env=None):
    """
    fit net with train_data for 10 epochs
    
    target  = torch model
    op_args = [train_data, epochs?]
    Returns trained model.
    """
    torch = _try_import("torch")
    nn = _try_import("torch.nn")

    epochs = 10
    for a in op_args:
        try:
            v = int(a)
            epochs = v
        except (ValueError, TypeError):
            pass

    train_data = op_args[0] if op_args else None

    if torch and nn and hasattr(target, "parameters"):
        import torch
        optimizer = torch.optim.Adam(target.parameters(), lr=0.001)
        loss_fn = nn.CrossEntropyLoss()
        print(f"[enlg DL] Training for {epochs} epoch(s)...")
        # If train_data is a DataLoader-compatible iterable, use it
        if hasattr(train_data, "__iter__") and not isinstance(train_data, str):
            for epoch in range(epochs):
                total_loss = 0
                for batch in train_data:
                    if isinstance(batch, (list, tuple)) and len(batch) == 2:
                        X, y = batch
                        optimizer.zero_grad()
                        out = target(X)
                        loss = loss_fn(out, y)
                        loss.backward()
                        optimizer.step()
                        total_loss += loss.item()
                print(f"  Epoch {epoch+1}/{epochs} — Loss: {total_loss:.4f}")
        else:
            print(f"  (Simulated training — no DataLoader provided)")
        print(f"[enlg DL] Training complete.")
        return target

    print(f"[enlg DL] Fit complete (stub — torch not available).")
    return target


# ─── DL_FORWARD ──────────────────────────────────────────────────────────────

def handle_dl_forward(target, op_args: list, env=None):
    """
    forward input using net
    
    target  = input tensor / array
    op_args = [model]
    Returns output tensor.
    """
    torch = _try_import("torch")
    model = op_args[0] if op_args else None

    if torch and model and hasattr(model, "forward"):
        import torch
        inp = target
        if not isinstance(inp, torch.Tensor):
            try:
                inp = torch.tensor(inp, dtype=torch.float32)
            except Exception:
                inp = torch.zeros(1, 128)
        with torch.no_grad():
            res = model(inp)
        print(f"[enlg DL] Forward pass output: {res}")
        return res

    print(f"[enlg DL] Forward pass (stub): {target}")
    return f"ForwardPass({target})"


# ─── DL_COMPILE registry alias for intents ───────────────────────────────────

DL_HANDLERS: dict = {
    "DL_COMPILE": handle_dl_compile,
    "DL_FIT":     handle_dl_fit,
    "DL_FORWARD": handle_dl_forward,
}
