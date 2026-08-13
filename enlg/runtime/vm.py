"""enlg Virtual Machine.

Executes a linear sequence of CIR opcodes against a runtime environment.
"""

import sys
import asyncio
import inspect
import importlib
from dataclasses import dataclass
from typing import Any, List
from enlg.compiler.cir import CIROpcode, CIRBlock
from enlg.runtime.environment import Environment

@dataclass
class EnlgFunction:
    """Represents a compiled enlg user function."""
    parameters: List[str]
    body: CIRBlock
    is_async: bool

@dataclass
class EnlgClass:
    """Represents a class blueprint."""
    name: str
    bases: List[str]
    environment: Environment  # Contains class methods and defaults

@dataclass
class EnlgInstance:
    """Represents an instantiated object."""
    enlg_class: EnlgClass
    environment: Environment  # Contains instance state

class VirtualMachine:
    """Stack-based execution engine."""
    
    def __init__(self, environment: Environment = None):
        self.environment = environment or Environment(load_builtins=True)
        self.stack: List[Any] = []
        self.return_value: Any = None
        self.exception_blocks: List[int] = []
        
    def execute(self, block: CIRBlock):
        """Executes a CIRBlock."""
        instructions = block.instructions
        ip = 0
        length = len(instructions)
        
        while ip < length:
            try:
                instr = instructions[ip]
                opcode = instr.opcode
                args = instr.args
                
                if opcode == CIROpcode.LOAD_CONST:
                    val = args[0]
                    if isinstance(val, str):
                        try:
                            num = float(val)
                            val = int(num) if num.is_integer() else num
                        except ValueError:
                            pass
                    self.stack.append(val)
                    ip += 1
                    
                elif opcode == CIROpcode.LOAD_VAR:
                    val = self.environment.get(args[0])
                    self.stack.append(val)
                    ip += 1
                    
                elif opcode == CIROpcode.STORE_VAR:
                    val = self.stack.pop()
                    self.environment.set(args[0], val)
                    ip += 1
                    
                elif opcode == CIROpcode.PRINT:
                    val = self.stack.pop()
                    # Use standard print, which goes to sys.stdout
                    print(val)
                    ip += 1
                    
                elif opcode == CIROpcode.JUMP:
                    ip = args[0]
                    
                elif opcode == CIROpcode.JUMP_IF_FALSE:
                    condition = self.stack.pop()
                    if not condition:
                        ip = args[0]
                    else:
                        ip += 1
                        
                elif opcode == CIROpcode.MAKE_FUNCTION:
                    func = EnlgFunction(parameters=args[0], body=args[1], is_async=args[2])
                    self.stack.append(func)
                    ip += 1
                    
                elif opcode == CIROpcode.CALL:
                    argc = args[0]
                    # Pop arguments in reverse order, then reverse them to original
                    call_args = [self.stack.pop() for _ in range(argc)]
                    call_args.reverse()
                    
                    target = self.stack.pop()
                    
                    if isinstance(target, EnlgFunction):
                        # Spawn new environment
                        local_env = Environment(parent=self.environment)
                        for param, arg_val in zip(target.parameters, call_args):
                            local_env.set(param, arg_val)
                            
                        # Spawn new VM instance
                        sub_vm = VirtualMachine(environment=local_env)
                        sub_vm.execute(target.body)
                        
                        self.stack.append(sub_vm.return_value)
                    elif callable(target):
                        # Native python function
                        def _coerce(item):
                            if isinstance(item, str):
                                try:
                                    num = float(item)
                                    return int(num) if num.is_integer() else num
                                except (ValueError, TypeError):
                                    return item
                            elif isinstance(item, list):
                                return [_coerce(x) for x in item]
                            return item

                        coerced_args = [_coerce(a) for a in call_args]
                        try:
                            res = target(*coerced_args)
                        except TypeError as te:
                            if "must be encoded" in str(te) or "bytes" in str(te):
                                bytes_args = [a.encode("utf-8") if isinstance(a, str) else a for a in coerced_args]
                                res = target(*bytes_args)
                                if hasattr(res, "hexdigest"):
                                    res = res.hexdigest()
                            else:
                                raise te
                        self.stack.append(res)
                    else:
                        raise TypeError(f"Object of type {type(target).__name__} is not callable")
                    ip += 1
                    
                elif opcode == CIROpcode.RETURN:
                    has_value = args[0]
                    if has_value:
                        self.return_value = self.stack.pop()
                    else:
                        self.return_value = None
                    # Exit the execute loop
                    return
                    
                elif opcode == CIROpcode.MAKE_CLASS:
                    name = args[0]
                    bases = args[1]
                    body = args[2]
                    
                    # Execute the class body in an isolated environment to collect its attributes
                    class_env = Environment(parent=self.environment)
                    class_vm = VirtualMachine(environment=class_env)
                    class_vm.execute(body)
                    
                    cls = EnlgClass(name=name, bases=bases, environment=class_env)
                    self.stack.append(cls)
                    ip += 1
                    
                elif opcode == CIROpcode.INSTANTIATE:
                    argc = args[0]
                    init_args = [self.stack.pop() for _ in range(argc)]
                    init_args.reverse()
                    
                    cls = self.stack.pop()
                    if not isinstance(cls, EnlgClass):
                        raise TypeError(f"Target is not a class blueprint: {cls}")
                        
                    # Create instance environment with class environment as parent (to inherit methods)
                    inst_env = Environment(parent=cls.environment)
                    
                    # Clone attributes from class env to instance env to avoid shared state mutations
                    for k, v in cls.environment.variables.items():
                        if not isinstance(v, EnlgFunction):
                            inst_env.set(k, v)
                    
                    instance = EnlgInstance(enlg_class=cls, environment=inst_env)
                    
                    # If we have an init function, we'd call it here
                    if "init" in cls.environment.variables:
                        init_func = cls.environment.get("init")
                        if isinstance(init_func, EnlgFunction):
                            init_env = Environment(parent=cls.environment)
                            init_env.set("self", instance)
                            for param, arg_val in zip(init_func.parameters, init_args):
                                init_env.set(param, arg_val)
                            sub_vm = VirtualMachine(environment=init_env)
                            sub_vm.execute(init_func.body)
                            
                    self.stack.append(instance)
                    ip += 1
                    
                elif opcode == CIROpcode.SETUP_ATTEMPT:
                    self.exception_blocks.append(args[0])
                    ip += 1
                    
                elif opcode == CIROpcode.END_ATTEMPT:
                    self.exception_blocks.pop()
                    ip += 1
                    
                elif opcode == CIROpcode.RAISE:
                    val = self.stack.pop()
                    # Throw it as a standard python Exception to be caught by the VM loop
                    raise Exception(val)
                    
                elif opcode == CIROpcode.AWAIT:
                    target = self.stack.pop()
                    if inspect.iscoroutine(target) or isinstance(target, asyncio.Future):
                        # Resolve coroutine / future synchronously in VM context
                        try:
                            loop = asyncio.get_running_loop()
                        except RuntimeError:
                            loop = None
                            
                        if loop and loop.is_running():
                            # If an event loop is already running, run it until complete in a sub-thread or task
                            import concurrent.futures
                            with concurrent.futures.ThreadPoolExecutor() as pool:
                                res = pool.submit(asyncio.run, target).result()
                        else:
                            res = asyncio.run(target)
                        self.stack.append(res)
                    else:
                        # Non-async target pass through
                        self.stack.append(target)
                    ip += 1
                    
                elif opcode == CIROpcode.IMPORT_MODULE:
                    mod_name = args[0]
                    # 1. Check enlg stdlib first (ml, dl, data, std, etc.)
                    _ENLG_STDLIB = {"ml", "dl", "data", "std"}
                    if mod_name in _ENLG_STDLIB:
                        try:
                            module = importlib.import_module(f"enlg.stdlib.{mod_name}")
                            # Expose all top-level symbols into environment
                            for attr in dir(module):
                                if not attr.startswith("_"):
                                    self.environment.set(attr, getattr(module, attr))
                            self.environment.set(mod_name, module)
                        except ImportError as e:
                            raise ImportError(f"Could not load enlg stdlib '{mod_name}': {e}")
                    else:
                        # 2. Fall back to native Python import
                        try:
                            module = importlib.import_module(mod_name)
                            self.environment.set(mod_name, module)
                        except ImportError as e:
                            raise ImportError(f"Could not import '{mod_name}': {e}")
                    ip += 1
                    
                elif opcode == CIROpcode.NATIVE_CALL:
                    target_path = args[0]
                    argc = args[1]
                    
                    call_args = [self.stack.pop() for _ in range(argc)]
                    call_args.reverse()
                    
                    parts = target_path.split(".")
                    # Resolve root symbol from environment
                    root_name = parts[0]
                    obj = self.environment.get(root_name)
                    
                    # Traverse attributes
                    for attr in parts[1:]:
                        obj = getattr(obj, attr)
                        
                    if callable(obj):
                        def _coerce(item):
                            if isinstance(item, str):
                                try:
                                    num = float(item)
                                    return int(num) if num.is_integer() else num
                                except (ValueError, TypeError):
                                    return item
                            elif isinstance(item, list):
                                return [_coerce(x) for x in item]
                            return item

                        coerced_args = [_coerce(a) for a in call_args]
                        res = obj(*coerced_args)
                        self.stack.append(res)
                    else:
                        # Attribute access without call
                        self.stack.append(obj)
                    ip += 1
                    
                elif opcode == CIROpcode.UNARY_OP:
                    op = args[0]
                    val = self.stack.pop()
                    if isinstance(val, str):
                        try:
                            num = float(val)
                            val = int(num) if num.is_integer() else num
                        except ValueError:
                            pass
                    if op == "-":
                        res = -val
                    elif op in ("!", "not"):
                        res = not bool(val)
                    elif op == "~":
                        res = ~int(val)
                    else:
                        raise NotImplementedError(f"Unary operator '{op}' not supported yet")
                    self.stack.append(res)
                    ip += 1
                    
                elif opcode == CIROpcode.BINARY_OP:
                    op = args[0]
                    right = self.stack.pop()
                    left = self.stack.pop()
                    
                    def _to_num(val):
                        if isinstance(val, str):
                            try:
                                num = float(val)
                                return int(num) if num.is_integer() else num
                            except ValueError:
                                return val
                        return val

                    left_c = _to_num(left)
                    right_c = _to_num(right)

                    if op == "+":
                        if isinstance(left_c, str) and isinstance(right_c, str):
                            res = left_c + right_c
                        else:
                            res = left_c + right_c
                    elif op == "-":
                        res = left_c - right_c
                    elif op == "*":
                        res = left_c * right_c
                    elif op == "/":
                        res = left_c / right_c
                    elif op == "%":
                        res = left_c % right_c
                    elif op == "**":
                        res = left_c ** right_c
                    elif op == "//":
                        res = left_c // right_c
                    elif op in ("&&", "and"):
                        res = bool(left_c and right_c)
                    elif op in ("||", "or"):
                        res = bool(left_c or right_c)
                    elif op == "&":
                        res = int(left_c) & int(right_c)
                    elif op == "|":
                        res = int(left_c) | int(right_c)
                    elif op == "^":
                        res = int(left_c) ^ int(right_c)
                    elif op == "<<":
                        res = int(left_c) << int(right_c)
                    elif op == ">>":
                        res = int(left_c) >> int(right_c)
                    elif op in ("is", "==", "equals", "equal to"):
                        res = (left_c == right_c)
                    elif op in ("is not", "!=", "not equal to"):
                        res = (left_c != right_c)
                    elif op in ("greater than", ">"):
                        res = left_c > right_c
                    elif op in ("less than", "<"):
                        res = left_c < right_c
                    elif op in ("greater than or equal to", ">="):
                        res = left_c >= right_c
                    elif op in ("less than or equal to", "<="):
                        res = left_c <= right_c
                    elif op == "[]":
                        # Indexing: left[right]
                        idx = int(right) if not isinstance(right, int) else right
                        res = left[idx]
                    else:
                        raise NotImplementedError(f"Binary operator '{op}' not supported yet")
                        
                    self.stack.append(res)
                    ip += 1
                    
                elif opcode == CIROpcode.BUILD_LIST:
                    count = args[0]
                    items = [self.stack.pop() for _ in range(count)]
                    items.reverse()
                    self.stack.append(items)
                    ip += 1
                    
                elif opcode == CIROpcode.BUILD_MAP:
                    count = args[0]
                    res_dict = {}
                    # Pop count key-value pairs
                    for _ in range(count):
                        v = self.stack.pop()
                        k = self.stack.pop()
                        res_dict[k] = v
                    self.stack.append(res_dict)
                    ip += 1
                    

                    
                else:
                    raise NotImplementedError(f"Opcode not supported in execution yet: {opcode}")
                    
            except Exception as e:
                if len(self.exception_blocks) > 0:
                    # Pop the innermost rescue target
                    target_ip = self.exception_blocks.pop()
                    # Extract the original enlg error value if it was raised manually, else use string
                    error_val = e.args[0] if len(e.args) > 0 else str(e)
                    self.stack.append(error_val)
                    ip = target_ip
                else:
                    # No rescue blocks registered, crash the VM
                    raise e
