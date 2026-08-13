"""enlg Unified Error Code Registry.

This module maps exact error codes to clear, actionable English descriptions,
ensuring the 'fail closed' philosophy is understandable to the developer.
"""

from typing import Dict

ERROR_MESSAGES: Dict[str, str] = {
    # Lexical & Syntax Errors (E1000 - E1999)
    "E1001": "Unrecognized character encountered during tokenization.",
    "E1002": "Unmatched scope or brackets.",
    "E1003": "Malformed expression before intent locking.",
    
    # Intent & Hint Errors (E2000 - E2999)
    "E2001": "Unknown hint keyword. The compiler could not determine what operation you intend to perform.",
    "E2002": "Ambiguous intent. The given phrasing matches multiple operations and cannot be resolved deterministically. Please rephrase.",
    "E2003": "Missing required arguments for the locked intent.",
    
    # Semantic & Type Errors (E3000 - E3999)
    "E3001": "Type mismatch. The provided value does not match the expected type.",
    "E3002": "Name resolution error. The referenced variable or function is not defined.",
    "E3003": "Invalid reassignment to a constant or immutable value.",
    
    # Module & Interop Errors (E4000 - E4999)
    "E4001": "Failed to import module.",
    "E4002": "Unsupported Python interoperability feature.",
    
    # Security & Boundary Errors (E5000 - E5999)
    "E5001": "Security violation: Attempted to perform an unsafe external evaluation.",
}

def get_error_message(code: str) -> str:
    """Retrieve the English explanation for a given error code."""
    return ERROR_MESSAGES.get(code, "An unknown error occurred.")
