"""CIR Generator.

Translates an English-oriented Abstract Syntax Tree (AST)
into a flattened, linear Compiler Intermediate Representation (CIR).
"""

from enlg.ast.nodes import (
    ASTNode, BlockNode, VariableDeclNode, AssignmentNode,
    OutputNode, LiteralNode, IdentifierNode, IfNode, WhileNode,
    FunctionDefNode, FunctionCallNode, ReturnNode, ClassDefNode,
    InstantiateNode, AttemptNode, RescueNode, RaiseNode, AwaitNode,
    ImportNode, PythonInteropNode, BinaryOpNode, UnaryOpNode,
    ListNode, MapNode, DomainOpNode, BooleanNode
)
from enlg.compiler.cir import CIROpcode, CIRInstruction, CIRBlock

class CIRGenerator:
    """Visits AST nodes and generates sequential CIR instructions."""
    
    def __init__(self):
        self.block = CIRBlock(instructions=[])
        
    def generate(self, node: ASTNode) -> CIRBlock:
        self.block = CIRBlock(instructions=[])
        self._visit(node)
        return self.block
        
    def _visit(self, node: ASTNode):
        method_name = f"_visit_{type(node).__name__}"
        visitor = getattr(self, method_name, self._generic_visit)
        visitor(node)
        
    def _generic_visit(self, node: ASTNode):
        raise NotImplementedError(f"No CIR generation defined for {type(node).__name__}")
        
    def _visit_BlockNode(self, node: BlockNode):
        for stmt in node.statements:
            self._visit(stmt)
            
    def _visit_LiteralNode(self, node: LiteralNode):
        self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [node.value]))
        
    def _visit_IdentifierNode(self, node: IdentifierNode):
        self.block.append(CIRInstruction(CIROpcode.LOAD_VAR, [node.name]))
        
    def _visit_VariableDeclNode(self, node: VariableDeclNode):
        if node.value:
            self._visit(node.value)
        self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.identifier]))
        
    def _visit_AssignmentNode(self, node: AssignmentNode):
        self._visit(node.value)
        self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.identifier]))
        
    def _visit_OutputNode(self, node: OutputNode):
        self._visit(node.expression)
        self.block.append(CIRInstruction(CIROpcode.PRINT, []))

    def _visit_IfNode(self, node: 'IfNode'):
        # 1. Evaluate Condition
        self._visit(node.condition)
        
        # 2. Emit placeholder jump
        jump_instr = CIRInstruction(CIROpcode.JUMP_IF_FALSE, [-1])
        self.block.append(jump_instr)
        
        # 3. Generate True Body
        self._visit(node.body)
        
        # 4. Handle Else block (not fully implemented in Phase 5 parser, but preparing struct)
        if hasattr(node, 'else_body') and node.else_body:
            jump_end_instr = CIRInstruction(CIROpcode.JUMP, [-1])
            self.block.append(jump_end_instr)
            
            # Patch false jump to start of else body
            jump_instr.args[0] = len(self.block.instructions)
            
            self._visit(node.else_body)
            # Patch end jump to end of else body
            jump_end_instr.args[0] = len(self.block.instructions)
        else:
            # Patch false jump to end of true body
            jump_instr.args[0] = len(self.block.instructions)

    def _visit_WhileNode(self, node: 'WhileNode'):
        # 1. Mark start of condition
        start_idx = len(self.block.instructions)
        
        # 2. Evaluate Condition
        self._visit(node.condition)
        
        # 3. Emit placeholder exit jump
        jump_exit = CIRInstruction(CIROpcode.JUMP_IF_FALSE, [-1])
        self.block.append(jump_exit)
        
        # 4. Generate Loop Body
        self._visit(node.body)
        
        # 5. Jump back to start
        self.block.append(CIRInstruction(CIROpcode.JUMP, [start_idx]))
        
        # 6. Patch exit jump
        jump_exit.args[0] = len(self.block.instructions)

    def _visit_FunctionDefNode(self, node: 'FunctionDefNode'):
        # 1. Compile the body into an isolated block
        isolated_gen = CIRGenerator()
        isolated_block = isolated_gen.generate(node.body)
        
        # 2. Emit MAKE_FUNCTION
        self.block.append(CIRInstruction(CIROpcode.MAKE_FUNCTION, [node.parameters, isolated_block, node.is_async]))
        
        # 3. Bind to variable name
        self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.name]))

    def _visit_FunctionCallNode(self, node: 'FunctionCallNode'):
        # 1. Push function object
        self.block.append(CIRInstruction(CIROpcode.LOAD_VAR, [node.name]))
        
        # 2. Push arguments to stack (in order)
        for arg in node.arguments:
            self._visit(arg)
            
        # 3. Call
        self.block.append(CIRInstruction(CIROpcode.CALL, [len(node.arguments)]))

    def _visit_ReturnNode(self, node: 'ReturnNode'):
        if node.expression:
            self._visit(node.expression)
            self.block.append(CIRInstruction(CIROpcode.RETURN, [True]))
        else:
            self.block.append(CIRInstruction(CIROpcode.RETURN, [False]))

    def _visit_ClassDefNode(self, node: 'ClassDefNode'):
        # 1. Compile the body into an isolated block
        isolated_gen = CIRGenerator()
        isolated_block = isolated_gen.generate(node.body)
        
        # 2. Emit MAKE_CLASS
        self.block.append(CIRInstruction(CIROpcode.MAKE_CLASS, [node.name, node.base_classes, isolated_block]))
        
        # 3. Bind to variable name
        self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.name]))

    def _visit_InstantiateNode(self, node: 'InstantiateNode'):
        # 1. Push class object
        self.block.append(CIRInstruction(CIROpcode.LOAD_VAR, [node.class_name]))
        
        # 2. Push arguments to stack (in order)
        for arg in node.arguments:
            self._visit(arg)
            
        # 3. Instantiate
        self.block.append(CIRInstruction(CIROpcode.INSTANTIATE, [len(node.arguments)]))

    def _visit_AttemptNode(self, node: 'AttemptNode'):
        # 1. Emit SETUP_ATTEMPT with dummy target
        setup_instr = CIRInstruction(CIROpcode.SETUP_ATTEMPT, [-1])
        self.block.append(setup_instr)
        
        # 2. Generate try body
        self._visit(node.body)
        
        # 3. Emit END_ATTEMPT (pops exception target if success)
        self.block.append(CIRInstruction(CIROpcode.END_ATTEMPT, []))
        
        # 4. Jump past rescue block
        jump_instr = CIRInstruction(CIROpcode.JUMP, [-1])
        self.block.append(jump_instr)
        
        # Store state for RescueNode
        if not hasattr(self, '_active_attempts'):
            self._active_attempts = []
        self._active_attempts.append((setup_instr, jump_instr))

    def _visit_RescueNode(self, node: 'RescueNode'):
        setup_instr, jump_instr = self._active_attempts.pop()
        
        # 5. Back-patch SETUP_ATTEMPT target to start of rescue
        setup_instr.args[0] = len(self.block.instructions)
        
        # 6. Bind caught error to variable
        self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.error_name]))
        
        # 7. Generate rescue body
        self._visit(node.body)
        
        # 8. Back-patch JUMP target
        jump_instr.args[0] = len(self.block.instructions)

    def _visit_RaiseNode(self, node: 'RaiseNode'):
        # 1. Evaluate expression to throw
        self._visit(node.expression)
        
        # 2. Emit RAISE
        self.block.append(CIRInstruction(CIROpcode.RAISE, []))

    def _visit_AwaitNode(self, node: 'AwaitNode'):
        # 1. Evaluate target expression
        self._visit(node.expression)
        
        # 2. Emit AWAIT
        self.block.append(CIRInstruction(CIROpcode.AWAIT, []))

    def _visit_ImportNode(self, node: 'ImportNode'):
        # 1. Emit IMPORT_MODULE
        self.block.append(CIRInstruction(CIROpcode.IMPORT_MODULE, [node.module]))

    def _visit_PythonInteropNode(self, node: 'PythonInteropNode'):
        # 1. Evaluate arguments (in order)
        for arg in node.arguments:
            self._visit(arg)
            
        # 2. Emit NATIVE_CALL
        self.block.append(CIRInstruction(CIROpcode.NATIVE_CALL, [node.target, len(node.arguments)]))

    def _visit_BinaryOpNode(self, node: 'BinaryOpNode'):
        # 1. Visit left operand
        self._visit(node.left)
        
        # 2. Visit right operand
        self._visit(node.right)
        
        # 3. Emit BINARY_OP opcode
        self.block.append(CIRInstruction(CIROpcode.BINARY_OP, [node.op]))

    def _visit_UnaryOpNode(self, node: 'UnaryOpNode'):
        # 1. Visit operand
        self._visit(node.operand)
        
        # 2. Emit UNARY_OP opcode
        self.block.append(CIRInstruction(CIROpcode.UNARY_OP, [node.op]))

    def _visit_ListNode(self, node: 'ListNode'):
        for elem in node.elements:
            self._visit(elem)
        self.block.append(CIRInstruction(CIROpcode.BUILD_LIST, [len(node.elements)]))

    def _visit_MapNode(self, node: 'MapNode'):
        for k, v in node.pairs.items():
            self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [k]))
            self._visit(v)
        self.block.append(CIRInstruction(CIROpcode.BUILD_MAP, [len(node.pairs)]))

    def _visit_DomainOpNode(self, node: 'DomainOpNode'):
        if node.store_result and node.op == "AI_SPLIT":
            # LOAD_VAR the source dataset (exists already)
            self._visit(node.target)
        elif node.store_result and node.store_back:
            # store_back=True: target var EXISTS — use LOAD_VAR to get current value
            self._visit(node.target)   # emits LOAD_VAR <model_var>
        elif node.store_result:
            # New variable being created — emit LOAD_CONST of var name string
            self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [node.store_result]))
        else:
            self._visit(node.target)

        # Visit arguments (not for AI_SPLIT — its args are dest var name nodes, not values)
        if node.op != "AI_SPLIT":
            for arg in node.arguments:
                self._visit(arg)

        # Dynamic Context-Aware Scope Resolution
        DOMAIN_MAP = {
            "AI_LOAD": ("ml", "load"),
            "AI_PREPROCESS": ("ml", "preprocess"),
            "AI_SPLIT": ("ml", "split"),
            "AI_TRAIN": ("ml", "train"),
            "AI_FIT": ("ml", "fit"),
            "AI_PREDICT": ("ml", "predict"),
            "AI_EVALUATE": ("ml", "evaluate"),
            "AI_SAVE": ("ml", "save"),
            "AI_RESTORE": ("ml", "restore"),
            "CYBER_ENCRYPT": ("cyber", "encrypt"),
            "CYBER_SCAN": ("cyber", "scan"),
            "SEC_ENCRYPT": ("cyber", "encrypt"),
            "SEC_SCAN": ("cyber", "scan"),
            "CLOUD_DEPLOY": ("cloud", "deploy"),
            "CLOUD_FETCH": ("cloud", "deploy"),
            "CLOUD_UPLOAD": ("cloud", "upload"),
        }
        
        default_mod, func_name = DOMAIN_MAP.get(node.op, ("ml", node.op.lower().split("_")[-1]))
        
        # If user explicitly specified a module via 'from <mod>' or 'using <mod>'
        mod_name = getattr(node, 'from_module', None) or default_mod
        
        # 1. Ensure the module is imported
        self.block.append(CIRInstruction(CIROpcode.IMPORT_MODULE, [mod_name]))
        
        # 2. Emit NATIVE_CALL targetting the function within the module
        # argc includes the target + any arguments (unless it's AI_SPLIT which has special argument handling)
        argc = 1 if node.op == "AI_SPLIT" else len(node.arguments) + 1
        self.block.append(CIRInstruction(CIROpcode.NATIVE_CALL, [f"{mod_name}.{func_name}", argc]))

        # Store result
        if node.store_result:
            if node.op == "AI_SPLIT":
                parts = node.store_result.split(":")
                if len(parts) >= 2:
                    train_var, test_var = parts[0], parts[1]
                    self.block.append(CIRInstruction(CIROpcode.STORE_VAR, ["_split_result"]))
                    self.block.append(CIRInstruction(CIROpcode.LOAD_VAR, ["_split_result"]))
                    self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [0]))
                    self.block.append(CIRInstruction(CIROpcode.BINARY_OP, ["[]"]))
                    self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [train_var]))
                    self.block.append(CIRInstruction(CIROpcode.LOAD_VAR, ["_split_result"]))
                    self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [1]))
                    self.block.append(CIRInstruction(CIROpcode.BINARY_OP, ["[]"]))
                    self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [test_var]))
                else:
                    self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [parts[0]]))
            else:
                # Single var store (load, preprocess, restore, train store-back)
                self.block.append(CIRInstruction(CIROpcode.STORE_VAR, [node.store_result]))

    def _visit_BooleanNode(self, node: 'BooleanNode'):
        self.block.append(CIRInstruction(CIROpcode.LOAD_CONST, [node.value]))
