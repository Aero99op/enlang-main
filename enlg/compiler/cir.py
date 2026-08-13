"""Compiler Intermediate Representation (CIR).

Defines the low-level, flattened execution instruction set
used by the enlg runtime virtual machine.
"""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Any, List

class CIROpcode(Enum):
    # Constants & Variables
    LOAD_CONST = auto()   # (value) -> Pushes constant to stack
    LOAD_VAR = auto()     # (name) -> Pushes variable value to stack
    STORE_VAR = auto()    # (name) -> Pops stack and stores in variable
    
    # I/O
    PRINT = auto()        # () -> Pops stack and prints it
    
    # Control Flow
    JUMP_IF_FALSE = auto() # (target_index) -> Pops stack, jumps if false
    JUMP = auto()          # (target_index) -> Jumps unconditionally
    
    # Functions
    MAKE_FUNCTION = auto() # (params: list, body: CIRBlock, is_async: bool) -> Pushes a callable object
    CALL = auto()          # (argc: int) -> Pops `argc` args, pops function, calls it, pushes result
    RETURN = auto()        # (has_value: bool) -> Returns from function execution
    
    # OOP
    MAKE_CLASS = auto()    # (name: str, bases: list, body: CIRBlock) -> Pushes a class blueprint
    INSTANTIATE = auto()   # (argc: int) -> Pops `argc` args, pops blueprint, creates instance, pushes instance
    LOAD_ATTR = auto()     # (attr: str) -> Pops object, pushes object.attr
    STORE_ATTR = auto()    # (attr: str) -> Pops value, pops object, sets object.attr = value
    
    # Exceptions
    SETUP_ATTEMPT = auto() # (target_index: int) -> Pushes rescue target to VM exception stack
    END_ATTEMPT = auto()   # () -> Pops target from VM exception stack (success path)
    RAISE = auto()         # () -> Pops value from data stack, throws it as error
    
    # Async
    AWAIT = auto()         # () -> Pops coroutine/future, awaits result, pushes to stack
    
    # Interop
    IMPORT_MODULE = auto() # (module_name: str) -> Imports python module into environment
    NATIVE_CALL = auto()   # (target: str, argc: int) -> Calls dot-separated python function
    
    # Operators & Data Structures
    BINARY_OP = auto()     # (op: str) -> Pops right, pops left, performs op, pushes result
    UNARY_OP = auto()      # (op: str) -> Pops val, performs unary op, pushes result
    BUILD_LIST = auto()    # (count: int) -> Pops `count` items, pushes list
    BUILD_MAP = auto()     # (count: int) -> Pops `count` pairs, pushes dict
    
    # Specialized Domains
    DOMAIN_OP = auto()     # (op: str, argc: int) -> Pops `argc` args, pops target, executes domain handler


@dataclass
class CIRInstruction:
    """A single executable instruction."""
    opcode: CIROpcode
    args: List[Any]
    
    def __repr__(self):
        args_str = ", ".join(repr(a) for a in self.args)
        return f"{self.opcode.name} {args_str}"

@dataclass
class CIRBlock:
    """A linear sequence of instructions."""
    instructions: List[CIRInstruction]
    
    def append(self, instr: CIRInstruction):
        self.instructions.append(instr)
