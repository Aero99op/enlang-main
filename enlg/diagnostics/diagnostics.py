"""enlg Diagnostic Error Model.

Defines the base error class and specific failure models for the compiler.
Adheres strictly to the 'fail closed' principle.
"""

from typing import Optional, List
from .error_codes import get_error_message
from .error_formatter import format_human_diagnostic


class EnlgError(Exception):
    """Base class for all enlg ecosystem errors."""
    def __init__(
        self,
        code: str,
        details: Optional[str] = None,
        line: Optional[int] = None,
        column: Optional[int] = None,
        file_path: Optional[str] = None,
        source_code: Optional[str] = None,
        what: Optional[str] = None,
        why: Optional[str] = None,
        suggestions: Optional[List[str]] = None,
        domain: str = "enlng"
    ):
        self.code = code
        self.message = get_error_message(code)
        self.details = details
        self.line = line
        self.column = column
        self.file_path = file_path
        self.source_code = source_code
        self.what = what
        self.why = why
        self.suggestions = suggestions
        self.domain = domain
        super().__init__(self.__str__())

    def to_human_diagnostic(self) -> str:
        return format_human_diagnostic(
            source=self.source_code or "",
            line=self.line,
            col=self.column,
            file_path=self.file_path,
            domain=self.domain,
            error_type=self.__class__.__name__,
            what=self.what or self.details or self.message,
            why=self.why,
            suggestions=self.suggestions,
            raw_error=self.details or self.message
        )

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
