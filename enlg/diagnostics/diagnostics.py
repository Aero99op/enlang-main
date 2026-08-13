"""enlg Diagnostic Error Model.

Defines the base error class and specific failure models for the compiler.
Adheres strictly to the 'fail closed' principle.
"""

from typing import Optional
from .error_codes import get_error_message


class EnlgError(Exception):
    """Base class for all enlg ecosystem errors."""
    def __init__(self, code: str, details: Optional[str] = None):
        self.code = code
        self.message = get_error_message(code)
        self.details = details
        super().__init__(self.__str__())

    def __str__(self) -> str:
        base = f"[{self.code}] {self.message}"
        if self.details:
            base += f"\nDetails: {self.details}"
        return base


class LexicalError(EnlgError):
    """Raised during tokenization failures."""
    pass


class SyntaxError(EnlgError):
    """Raised when basic grammar is malformed before intent is locked."""
    pass


class UnknownHintError(EnlgError):
    """Raised when a hint keyword cannot be identified."""
    def __init__(self, details: Optional[str] = None):
        super().__init__("E2001", details)


class IntentAmbiguityError(EnlgError):
    """Raised when a hint keyword maps to multiple conflicting operations."""
    def __init__(self, details: Optional[str] = None):
        super().__init__("E2002", details)


class SemanticError(EnlgError):
    """Raised when type, name, or scoping rules are violated."""
    pass


class SecurityError(EnlgError):
    """Raised when the execution boundary is violated."""
    def __init__(self, details: Optional[str] = None):
        super().__init__("E5001", details)
