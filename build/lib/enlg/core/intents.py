"""enlg Intent Registry.

Maps natural English hint aliases to deterministic canonical Intent IDs.
The core of the Hint-Keyword Intent discovery mechanism.
"""

from typing import Dict, Set

# The authoritative registry mapping any valid alias to its explicit semantic intent.
INTENT_REGISTRY: Dict[str, str] = {
    # ── Variable Declarations ───────────────────────────────────────────────
    "declare": "DECLARE_VARIABLE",
    "create": "DECLARE_VARIABLE",
    "initialize": "DECLARE_VARIABLE",
    "let": "DECLARE_VARIABLE",
    "define": "DECLARE_VARIABLE",

    # ── Assignments ─────────────────────────────────────────────────────────
    "set": "ASSIGN_VARIABLE",
    "update": "ASSIGN_VARIABLE",
    "assign": "ASSIGN_VARIABLE",
    "change": "ASSIGN_VARIABLE",
    "increase": "ASSIGN_COMPOUND_ADD",
    "decrease": "ASSIGN_COMPOUND_SUB",
    "multiply": "ASSIGN_COMPOUND_MUL",
    "divide": "ASSIGN_COMPOUND_DIV",

    # ── Output Display ───────────────────────────────────────────────────────
    "display": "OUTPUT_DISPLAY",
    "show": "OUTPUT_DISPLAY",
    "output": "OUTPUT_DISPLAY",
    "print": "OUTPUT_DISPLAY",

    # ── Loops ────────────────────────────────────────────────────────────────
    "repeat": "LOOP_REPEAT",
    "while": "LOOP_WHILE",

    # ── Conditionals ─────────────────────────────────────────────────────────
    "if": "COND_IF",
    "else if": "COND_ELIF",
    "elif": "COND_ELIF",
    "else": "COND_ELSE",

    # ── Functions ────────────────────────────────────────────────────────────
    "function": "FUNC_DEF",
    "routine": "FUNC_DEF",
    "procedure": "FUNC_DEF",
    "method": "FUNC_DEF",
    "call": "FUNC_CALL",
    "invoke": "FUNC_CALL",
    "run": "FUNC_CALL",
    "execute": "FUNC_CALL",
    "return": "FUNC_RETURN",
    "give back": "FUNC_RETURN",

    # ── Exceptions ───────────────────────────────────────────────────────────
    "attempt": "BLOCK_TRY",
    "try": "BLOCK_TRY",
    "rescue": "BLOCK_CATCH",
    "catch": "BLOCK_CATCH",
    "throw": "STMT_RAISE",
    "raise": "STMT_RAISE",

    # ── Modules ──────────────────────────────────────────────────────────────
    "import": "STMT_IMPORT",
    "include": "STMT_IMPORT",
    "require": "STMT_IMPORT",

    # ── OOP ──────────────────────────────────────────────────────────────────
    "class": "CLASS_DEF",
    "model": "CLASS_DEF",
    "blueprint": "CLASS_DEF",
    "new": "CLASS_NEW",
    "instantiate": "CLASS_NEW",

    # ── Async ────────────────────────────────────────────────────────────────
    "await": "ASYNC_AWAIT",
    "wait for": "ASYNC_AWAIT",

    # ── Python Interop ───────────────────────────────────────────────────────
    "interop": "PYTHON_INTEROP",
    "native": "PYTHON_INTEROP",
    "usingf": "PYTHON_INTEROP",

    # ════════════════════════════════════════════════════════════════════════
    # DOMAIN KEYWORDS — AI / ML / DL / DATA SCIENCE
    # These are FINAL. Do not add more. Use handler registry for new operations.
    # ════════════════════════════════════════════════════════════════════════

    # ── Data Pipeline ────────────────────────────────────────────────────────
    "load": "AI_LOAD",           # load data from "file.csv"
    "preprocess": "AI_PREPROCESS", # preprocess data
    "split": "AI_SPLIT",         # split data into train_set and test_set

    # ── Model Lifecycle ──────────────────────────────────────────────────────
    "train": "AI_TRAIN",         # train model with train_data
    "fit": "AI_FIT",             # fit model with train_data (DL alias)
    "predict": "AI_PREDICT",     # predict input using model
    "evaluate": "AI_EVALUATE",   # evaluate model using test_data
    "save": "AI_SAVE",           # save model to "model.pkl"
    "restore": "AI_RESTORE",     # restore model from "model.pkl"

    # ── Deep Learning ────────────────────────────────────────────────────────
    "compile": "DL_COMPILE",     # compile net with layers=[...]
    "forward": "DL_FORWARD",     # forward input using net

    # ════════════════════════════════════════════════════════════════════════
    # DOMAIN KEYWORDS — CYBERSECURITY
    # ════════════════════════════════════════════════════════════════════════
    "scan": "SEC_SCAN",          # scan target on port 80
    "encrypt": "SEC_ENCRYPT",    # encrypt payload using sha256

    # ════════════════════════════════════════════════════════════════════════
    # DOMAIN KEYWORDS — CLOUD
    # ════════════════════════════════════════════════════════════════════════
    "deploy": "CLOUD_DEPLOY",    # deploy service using config
    "cloud_fetch": "CLOUD_FETCH", # cloud_fetch data from bucket
}

# ─── Connectors / Filler words ───────────────────────────────────────────────
# Ignored during intent discovery so "train model WITH train_data" parses cleanly.
CONNECTORS: Set[str] = {
    "to", "in", "into", "with", "using", "as", "on", "at", "out",
    "from", "under", "where", "by", "of", "the", "a", "an",
    "for", "layers",
}
