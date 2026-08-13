"""enlg ML Standard Library — Classical Machine Learning.

Import this in your .enlg file via: import ml

Exposes ready-made model blueprints that work directly with
the train / predict / evaluate domain verbs.
"""

def _try_import(pkg: str):
    import importlib
    try:
        return importlib.import_module(pkg)
    except ImportError:
        return None


def _build_model_registry():
    registry = {}

    # ── Scikit-Learn Models ────────────────────────────────────────────────
    sklearn_nb = _try_import("sklearn.naive_bayes")
    sklearn_lr = _try_import("sklearn.linear_model")
    sklearn_svm = _try_import("sklearn.svm")
    sklearn_tree = _try_import("sklearn.tree")
    sklearn_ens = _try_import("sklearn.ensemble")
    sklearn_nn = _try_import("sklearn.neural_network")
    sklearn_knn = _try_import("sklearn.neighbors")
    sklearn_cluster = _try_import("sklearn.cluster")

    if sklearn_nb:
        registry["NaiveBayes"]        = sklearn_nb.GaussianNB
        registry["MultinomialNB"]     = sklearn_nb.MultinomialNB
        registry["BernoulliNB"]       = sklearn_nb.BernoulliNB

    if sklearn_lr:
        registry["LinearModel"]       = sklearn_lr.LinearRegression
        registry["LinearRegression"]  = sklearn_lr.LinearRegression
        registry["Logistic"]          = sklearn_lr.LogisticRegression
        registry["LogisticRegression"]= sklearn_lr.LogisticRegression
        registry["Ridge"]             = sklearn_lr.Ridge
        registry["Lasso"]             = sklearn_lr.Lasso

    if sklearn_svm:
        registry["SVM"]               = sklearn_svm.SVC
        registry["SVR"]               = sklearn_svm.SVR

    if sklearn_tree:
        registry["DecisionTree"]      = sklearn_tree.DecisionTreeClassifier
        registry["DecisionTreeRegressor"] = sklearn_tree.DecisionTreeRegressor

    if sklearn_ens:
        registry["RandomForest"]      = sklearn_ens.RandomForestClassifier
        registry["RandomForestRegressor"] = sklearn_ens.RandomForestRegressor
        registry["GradientBoosting"]  = sklearn_ens.GradientBoostingClassifier
        registry["XGBoost"]           = sklearn_ens.GradientBoostingClassifier  # stdlib fallback

    if sklearn_nn:
        registry["MLP"]               = sklearn_nn.MLPClassifier
        registry["MLPRegressor"]      = sklearn_nn.MLPRegressor

    if sklearn_knn:
        registry["KNN"]               = sklearn_knn.KNeighborsClassifier
        registry["KNNRegressor"]      = sklearn_knn.KNeighborsRegressor

    if sklearn_cluster:
        registry["KMeans"]            = sklearn_cluster.KMeans
        registry["DBSCAN"]            = sklearn_cluster.DBSCAN

    return registry


# Build and expose registry
_MODEL_REGISTRY = _build_model_registry()

# Expose as top-level attributes so `import ml; declare m as NaiveBayes` works
for _name, _cls in _MODEL_REGISTRY.items():
    globals()[_name] = _cls

# Also expose the registry for runtime lookup
MODEL_REGISTRY = _MODEL_REGISTRY

print(f"[enlg ML] Standard Library loaded — {len(_MODEL_REGISTRY)} models available.")


# --- Zero-Domain Core Wrappers ---
import enlg.runtime.domain_handlers.ml_handlers as _ml

def train(target, *args):
    return _ml.handle_ai_train(target, list(args))

def load(target, *args):
    return _ml.handle_ai_load(target, list(args))

def preprocess(target, *args):
    return _ml.handle_ai_preprocess(target, list(args))

def split(target, *args):
    return _ml.handle_ai_split(target, list(args))

def predict(target, *args):
    return _ml.handle_ai_predict(target, list(args))

def evaluate(target, *args):
    return _ml.handle_ai_evaluate(target, list(args))

def save(target, *args):
    return _ml.handle_ai_save(target, list(args))

def restore(target, *args):
    return _ml.handle_ai_restore(target, list(args))

def fit(target, *args):
    return _ml.handle_ai_fit(target, list(args))
