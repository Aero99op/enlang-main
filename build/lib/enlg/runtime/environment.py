"""Runtime Environment.

Manages variable scoping and memory resolution for the enlg virtual machine.
Supports hierarchical scopes (e.g., global, function-local).
"""

from typing import Any, Optional
from enlg.runtime.builtins import BUILTINS

class Environment:
    """A scoped memory space mapping identifiers to values."""
    
    def __init__(self, parent: Optional['Environment'] = None, load_builtins: bool = False):
        self.variables: dict[str, Any] = {}
        self.parent = parent
        
        if load_builtins:
            # Inject native helper functions into this environment
            self.variables.update(BUILTINS)
            
    def get(self, name: str) -> Any:
        """Retrieve a variable's value from the current or parent scopes."""
        if name in self.variables:
            return self.variables[name]

        if "." in name:
            parts = name.split(".")
            val = None
            prefix_idx = 1
            while prefix_idx <= len(parts):
                prefix = ".".join(parts[:prefix_idx])
                if prefix in self.variables:
                    val = self.variables[prefix]
                    break
                elif self.parent and prefix in self.parent.variables:
                    val = self.parent.variables[prefix]
                    break
                prefix_idx += 1
                
            if val is not None:
                for attr in parts[prefix_idx:]:
                    val = getattr(val, attr)
                return val
        elif self.parent:
            return self.parent.get(name)
        else:
            raise NameError(f"Undefined variable '{name}'")
            
    def set(self, name: str, value: Any):
        """Set a variable's value in the current scope."""
        self.variables[name] = value
