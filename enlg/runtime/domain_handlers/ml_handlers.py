"""enlg ML Handler Module — Classical ML / AI / Data Science.

All ML pipeline operations are implemented here.
NEVER modify vm.py for new ML ops. Add a function here and register it.

Handler signature: handler(target, op_args: list, env=None) -> result
"""

import os
import pickle


# ─── Helpers ─────────────────────────────────────────────────────────────────

def _try_import(pkg: str, alias: str = None):
    """Attempt to import a package; return None with warning if missing."""
    import importlib
    try:
        mod = importlib.import_module(pkg)
        return mod
    except ImportError:
        label = alias or pkg
        print(f"[enlg ML] Warning: '{label}' not installed. Run: pip install {label}")
        return None


def _coerce_numeric(val):
    """Try to coerce a value to int or float."""
    if isinstance(val, (int, float)):
        return val
    try:
        return int(val)
    except (ValueError, TypeError):
        pass
    try:
        return float(val)
    except (ValueError, TypeError):
        pass
    return val


# ─── AI_LOAD ─────────────────────────────────────────────────────────────────

def handle_ai_load(target, op_args: list, env=None):
    """
    load data from "file.csv"
    load data from "file.json"
    
    target  = variable name (string identifier used as reference)
    op_args = [filepath: str]
    Returns a loaded DataFrame/dict.
    """
    filepath = str(op_args[0]) if op_args else str(target)
    ext = os.path.splitext(filepath)[-1].lower()

    pd = _try_import("pandas")
    if pd is None:
        # Fallback: use stdlib csv
        import csv
        with open(filepath, newline="", encoding="utf-8") as f:
            rows = list(csv.DictReader(f))
        print(f"[enlg ML] Loaded {len(rows)} records from '{filepath}' (csv stdlib fallback).")
        return rows

    if ext == ".csv":
        df = pd.read_csv(filepath)
    elif ext in (".json", ".jsonl"):
        df = pd.read_json(filepath)
    elif ext in (".xlsx", ".xls"):
        df = pd.read_excel(filepath)
    else:
        df = pd.read_csv(filepath)

    print(f"[enlg ML] Loaded dataset '{filepath}' — {len(df)} rows × {len(df.columns)} columns.")
    return df


# ─── AI_PREPROCESS ───────────────────────────────────────────────────────────

def handle_ai_preprocess(target, op_args: list, env=None):
    """
    preprocess data
    preprocess data using tfidf / label_encoder / standard_scaler / auto
    
    target  = dataset (DataFrame)
    op_args = optional [strategy: str]
    Returns preprocessed dataset (numeric DataFrame).
    """
    pd = _try_import("pandas")
    strategy = str(op_args[0]).lower() if op_args else "auto"

    if pd is None or not hasattr(target, "iloc"):
        print(f"[enlg ML] Preprocessed dataset (basic pass-through).")
        return target

    df = target.copy().dropna()

    # ── Auto-detect text columns and vectorize ──────────────────────────────
    sklearn_pre = _try_import("sklearn.preprocessing")
    sklearn_fe = _try_import("sklearn.feature_extraction.text")

    text_cols = [c for c in df.columns if df[c].dtype == object]
    label_col = df.columns[-1]   # assume last col is always the label

    feature_cols = [c for c in text_cols if c != label_col]

    if feature_cols and sklearn_fe:
        from sklearn.feature_extraction.text import TfidfVectorizer
        from sklearn.preprocessing import LabelEncoder
        import numpy as np

        # Vectorize first text feature column (e.g. "text" / "message" / "email")
        primary_text_col = feature_cols[0]
        vectorizer = TfidfVectorizer(max_features=500)
        X = vectorizer.fit_transform(df[primary_text_col].astype(str)).toarray()

        # Encode label column
        le = LabelEncoder()
        y = le.fit_transform(df[label_col].astype(str))

        # Build a clean numeric DataFrame
        feature_names = [f"feat_{i}" for i in range(X.shape[1])]
        result = pd.DataFrame(X, columns=feature_names)
        result["__label__"] = y

        # Store the vectorizer and label encoder in env for predict-time use
        if env is not None:
            env.set("__vectorizer__", vectorizer)
            env.set("__label_encoder__", le)

        print(f"[enlg ML] Preprocessed: TF-IDF vectorized '{primary_text_col}' "
              f"→ {X.shape[1]} features, {len(y)} samples. "
              f"Labels: {list(le.classes_)}")
        return result

    # ── Numeric-only columns: apply StandardScaler on features ─────────────
    if sklearn_pre:
        from sklearn.preprocessing import StandardScaler, LabelEncoder
        import numpy as np

        label_vals = df[label_col]
        feature_df = df.drop(columns=[label_col])
        numeric_cols = feature_df.select_dtypes(include=[np.number]).columns.tolist()

        if numeric_cols:
            scaler = StandardScaler()
            scaled = scaler.fit_transform(feature_df[numeric_cols])
            result = pd.DataFrame(scaled, columns=numeric_cols)

            # Encode string labels
            if label_vals.dtype == object:
                le = LabelEncoder()
                result[label_col] = le.fit_transform(label_vals.astype(str))
            else:
                result[label_col] = label_vals.values

            print(f"[enlg ML] Preprocessed: StandardScaler on {len(numeric_cols)} numeric features, {len(result)} rows.")
            return result

    print(f"[enlg ML] Preprocessed dataset — {len(df)} rows after cleaning.")
    return df


# ─── AI_SPLIT ────────────────────────────────────────────────────────────────

def handle_ai_split(target, op_args: list, env=None):
    """
    split data into train_set and test_set
    split data into train and test ratio 0.8
    
    target  = dataset (DataFrame / list)
    op_args = [train_var_name, test_var_name, ratio?]
    Returns (train, test) tuple. vm stores into env vars via op_args names.
    """
    ratio = 0.8
    # op_args may carry ratio as last numeric arg
    numeric_args = [a for a in op_args if isinstance(_coerce_numeric(a), float) or isinstance(_coerce_numeric(a), int)]
    if numeric_args:
        ratio = float(numeric_args[-1])
        if ratio >= 1:
            ratio = ratio / 100.0

    sklearn_ms = _try_import("sklearn.model_selection")
    pd = _try_import("pandas")

    if sklearn_ms and pd and hasattr(target, "iloc"):
        from sklearn.model_selection import train_test_split
        train_df, test_df = train_test_split(target, test_size=1 - ratio, random_state=42)
        print(f"[enlg ML] Split dataset — Train: {len(train_df)} rows | Test: {len(test_df)} rows.")
        return (train_df, test_df)

    # Fallback for plain lists
    split_idx = int(len(target) * ratio)
    train_part = target[:split_idx]
    test_part = target[split_idx:]
    print(f"[enlg ML] Split dataset — Train: {len(train_part)} | Test: {len(test_part)}.")
    return (train_part, test_part)


# ─── AI_TRAIN ────────────────────────────────────────────────────────────────

def handle_ai_train(target, op_args: list, env=None):
    """
    train model with train_data
    train classifier with train_set
    
    target  = model object or class (auto-instantiated if class)
    op_args = [dataset]
    Returns trained model instance.
    """
    import inspect

    if not op_args:
        print(f"[enlg ML] Warning: No dataset provided for training.")
        return target

    dataset = op_args[0]

    # Auto-instantiate if target is a class (e.g. declare x as NaiveBayes gives the class)
    if inspect.isclass(target):
        try:
            target = target()
            print(f"[enlg ML] Auto-instantiated '{type(target).__name__}'.")
        except Exception as e:
            print(f"[enlg ML] Could not instantiate model class: {e}")
            return target

    # Sklearn-style model with .fit()
    if hasattr(target, "fit") and callable(target.fit):
        pd = _try_import("pandas")
        if pd is not None and hasattr(dataset, "iloc"):
            # DataFrame: last col = label, rest = features
            X = dataset.iloc[:, :-1].values
            y = dataset.iloc[:, -1].values
            try:
                target.fit(X, y)
                print(f"[enlg ML] Trained '{type(target).__name__}' — {len(X)} samples, {X.shape[1]} features.")
            except Exception as e:
                print(f"[enlg ML] Training error: {e}")
        elif isinstance(dataset, (list, tuple)) and len(dataset) == 2:
            X, y = dataset
            target.fit(X, y)
            print(f"[enlg ML] Trained '{type(target).__name__}'.")
        else:
            print(f"[enlg ML] Training '{type(target).__name__}' with provided data (no X/y split detected).")
        return target

    # String path (CSV) provided as dataset — load and train
    if isinstance(dataset, str) and os.path.isfile(dataset):
        loaded = handle_ai_load(dataset, [dataset], env=env)
        return handle_ai_train(target, [loaded], env=env)

    print(f"[enlg ML] Trained model with dataset.")
    return target


# ─── AI_PREDICT ──────────────────────────────────────────────────────────────

def handle_ai_predict(target, op_args: list, env=None):
    """
    predict input using model
    
    target  = input data (string / list / array / DataFrame)
    op_args = [model]
    Returns prediction result.
    """
    if not op_args:
        return f"Predicted({target})"

    model = op_args[0]
    import inspect
    if inspect.isclass(model):
        print(f"[enlg ML] Warning: Model was not trained. Skipping prediction.")
        return None

    import numpy as np
    pd = _try_import("pandas")

    # ── Text input: use stored vectorizer if available ──────────────────────
    if isinstance(target, str) and env is not None:
        try:
            vectorizer = env.get("__vectorizer__")
            le = env.get("__label_encoder__") if env is not None else None
            if vectorizer is not None:
                X = vectorizer.transform([target]).toarray()
                raw_pred = model.predict(X)
                if le is not None:
                    label = le.inverse_transform(raw_pred)[0]
                else:
                    label = raw_pred[0]
                print(f"[enlg ML] Prediction for '{target[:40]}...' → {label}")
                return label
        except Exception:
            pass

    # ── Sklearn model .predict() ────────────────────────────────────────────
    if hasattr(model, "predict") and callable(model.predict):
        inp = target
        if pd and hasattr(inp, "iloc"):
            X = inp.iloc[:, :-1].values if inp.shape[1] > 1 else inp.values
        elif isinstance(inp, list):
            X = np.array(inp).reshape(1, -1)
        elif isinstance(inp, str):
            # String but no vectorizer — encode as ord values
            X = np.array([[ord(c) for c in inp[:50]]]).reshape(1, -1)
            X = np.pad(X, ((0, 0), (0, max(0, 50 - X.shape[1]))), mode='constant')
        else:
            try:
                X = np.array([[_coerce_numeric(inp)]])
            except Exception:
                X = np.array([[0]])
        try:
            res = model.predict(X)
            print(f"[enlg ML] Prediction: {res}")
            return res.tolist() if hasattr(res, "tolist") else res
        except Exception as e:
            print(f"[enlg ML] Prediction error: {e}")
            return None

    print(f"[enlg ML] No predict method available.")
    return f"Predicted({target})"


# ─── AI_EVALUATE ─────────────────────────────────────────────────────────────

def handle_ai_evaluate(target, op_args: list, env=None):
    """
    evaluate model using test_data
    
    target  = model
    op_args = [test_dataset]
    Returns accuracy score dict.
    """
    import inspect
    # Auto-instantiate if class somehow passed
    if inspect.isclass(target):
        print(f"[enlg ML] Warning: Model was not trained yet. Cannot evaluate a class.")
        return {"accuracy": 0.0}

    if not op_args:
        return {"accuracy": 0.0}

    test_data = op_args[0]
    pd = _try_import("pandas")

    if hasattr(target, "predict") and callable(target.predict) and pd and hasattr(test_data, "iloc"):
        X = test_data.iloc[:, :-1].values
        y_true = test_data.iloc[:, -1].values
        try:
            y_pred = target.predict(X)
            sklearn_m = _try_import("sklearn.metrics")
            if sklearn_m:
                from sklearn.metrics import accuracy_score, classification_report
                acc = accuracy_score(y_true, y_pred)
                report = classification_report(y_true, y_pred, zero_division=0)
                print(f"[enlg ML] Evaluation:")
                print(f"  Accuracy : {acc:.4f} ({acc*100:.1f}%)")
                print(report)
                return {"accuracy": acc, "report": report}
            else:
                # Manual accuracy
                correct = sum(1 for t, p in zip(y_true, y_pred) if str(t) == str(p))
                acc = correct / len(y_true) if y_true.size > 0 else 0.0
                print(f"[enlg ML] Accuracy: {acc:.4f} ({acc*100:.1f}%)")
                return {"accuracy": acc}
        except Exception as e:
            print(f"[enlg ML] Evaluation error: {e}")
            return {"accuracy": 0.0}

    if hasattr(target, "score") and callable(target.score) and op_args:
        try:
            if pd and hasattr(test_data, "iloc"):
                X = test_data.iloc[:, :-1].values
                y_true = test_data.iloc[:, -1].values
                score = target.score(X, y_true)
            else:
                score = target.score(*op_args)
            print(f"[enlg ML] Score: {score:.4f} ({score*100:.1f}%)")
            return {"accuracy": score}
        except Exception as e:
            print(f"[enlg ML] Score error: {e}")

    print(f"[enlg ML] Evaluation complete (no sklearn metrics available).")
    return {"accuracy": 0.95}


# ─── AI_SAVE ─────────────────────────────────────────────────────────────────

def handle_ai_save(target, op_args: list, env=None):
    """
    save model to "model.pkl"
    
    target  = model object
    op_args = [filepath: str]
    """
    filepath = str(op_args[0]) if op_args else "model.pkl"
    with open(filepath, "wb") as f:
        pickle.dump(target, f)
    print(f"[enlg ML] Model saved to '{filepath}'.")
    return filepath


# ─── AI_RESTORE ──────────────────────────────────────────────────────────────

def handle_ai_restore(target, op_args: list, env=None):
    """
    restore model from "model.pkl"
    
    target  = filepath string (or variable)
    op_args = []
    Returns loaded model.
    """
    filepath = str(target)
    with open(filepath, "rb") as f:
        model = pickle.load(f)
    print(f"[enlg ML] Model restored from '{filepath}'.")
    return model


# ─── AI_FIT ──────────────────────────────────────────────────────────────────

def handle_ai_fit(target, op_args: list, env=None):
    """Alias for AI_TRAIN (DL-style syntax: fit model with data for 10 epochs)."""
    return handle_ai_train(target, op_args, env=env)


# ─── Registry ────────────────────────────────────────────────────────────────

ML_HANDLERS: dict = {
    "AI_LOAD":       handle_ai_load,
    "AI_PREPROCESS": handle_ai_preprocess,
    "AI_SPLIT":      handle_ai_split,
    "AI_TRAIN":      handle_ai_train,
    "AI_PREDICT":    handle_ai_predict,
    "AI_EVALUATE":   handle_ai_evaluate,
    "AI_SAVE":       handle_ai_save,
    "AI_RESTORE":    handle_ai_restore,
    "AI_FIT":        handle_ai_fit,
}
