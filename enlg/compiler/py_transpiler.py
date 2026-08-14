"""enlg to Python Transpiler.

Translates an Enlang (.enlg) AST / source code into clean, valid, standard Python (.py) code.
"""

from typing import Optional, List
from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.ast.nodes import (
    ASTNode, BlockNode, StatementNode, ExpressionNode,
    VariableDeclNode, AssignmentNode, OutputNode, LiteralNode, IdentifierNode,
    BinaryOpNode, UnaryOpNode, BooleanNode, NullNode, ListNode, MapNode,
    IfNode, WhileNode, ForNode, FunctionDefNode, FunctionCallNode, ReturnNode,
    AttemptNode, RescueNode, RaiseNode, ImportNode, ClassDefNode, InstantiateNode,
    PythonInteropNode, DomainOpNode
)

# Operator conversion map from Enlang (English/Symbolic) to standard Python
OP_MAP = {
    "less than": "<",
    "greater than": ">",
    "less than or equal to": "<=",
    "greater than or equal to": ">=",
    "equal to": "==",
    "equals": "==",
    "equal": "==",
    "is": "==",
    "not equal to": "!=",
    "is not": "!=",
    "and": "and",
    "or": "or",
    "not": "not",
    "==": "==",
    "!=": "!=",
    "<": "<",
    ">": ">",
    "<=": "<=",
    ">=": ">=",
    "+": "+",
    "-": "-",
    "*": "*",
    "/": "/",
    "%": "%",
    "**": "**",
    "//": "//",
    "&": "&",
    "|": "|",
    "^": "^",
    "<<": "<<",
    ">>": ">>",
}

class PythonTranspiler:
    """Visits Enlang AST nodes and generates formatted Python source code."""

    def __init__(self):
        self.indent_level = 0
        self.header_imports: List[str] = []

    def transpile(self, node: ASTNode) -> str:
        """Transpiles an AST node (typically BlockNode) to Python source code."""
        self.indent_level = 0
        self.header_imports = []
        body_code = self._visit(node)
        
        headers = ""
        if self.header_imports:
            # Deduplicate while preserving order
            unique_imports = list(dict.fromkeys(self.header_imports))
            headers = "\n".join(unique_imports) + "\n\n"
            
        return headers + body_code.strip() + "\n"

    def _indent(self) -> str:
        return "    " * self.indent_level

    def _visit(self, node: ASTNode) -> str:
        if node is None:
            return ""
        method_name = f"_visit_{type(node).__name__}"
        visitor = getattr(self, method_name, self._generic_visit)
        return visitor(node)

    def _generic_visit(self, node: ASTNode) -> str:
        return f"# [Transpiler Warning] Unhandled node: {type(node).__name__}"

    def _visit_BlockNode(self, node: BlockNode) -> str:
        lines = []
        for stmt in node.statements:
            code = self._visit(stmt)
            if code:
                lines.append(code)
        if not lines:
            return f"{self._indent()}pass"
        return "\n".join(lines)

    def _visit_LiteralNode(self, node: LiteralNode) -> str:
        if node.type_name == "string":
            # Cleanly format string
            escaped = node.value.replace("\\", "\\\\").replace('"', '\\"')
            return f'"{escaped}"'
        elif node.type_name == "boolean":
            return "True" if str(node.value).lower() in ("true", "1") else "False"
        elif node.type_name == "null":
            return "None"
        return str(node.value)

    def _visit_BooleanNode(self, node: BooleanNode) -> str:
        return "True" if node.value else "False"

    def _visit_NullNode(self, _node: NullNode) -> str:
        return "None"

    def _visit_IdentifierNode(self, node: IdentifierNode) -> str:
        return node.name

    def _visit_ListNode(self, node: ListNode) -> str:
        elems = [self._visit(e) for e in node.elements]
        return f"[{', '.join(elems)}]"

    def _visit_MapNode(self, node: MapNode) -> str:
        pairs = [f'"{k}": {self._visit(v)}' for k, v in node.pairs.items()]
        return f"{{{', '.join(pairs)}}}"

    def _visit_BinaryOpNode(self, node: BinaryOpNode) -> str:
        left = self._visit(node.left)
        right = self._visit(node.right)
        op = OP_MAP.get(node.op.lower().strip(), node.op)
        return f"({left} {op} {right})"

    def _visit_UnaryOpNode(self, node: UnaryOpNode) -> str:
        operand = self._visit(node.operand)
        op = OP_MAP.get(node.op.lower().strip(), node.op)
        if op == "not":
            return f"(not {operand})"
        return f"({op}{operand})"

    def _visit_VariableDeclNode(self, node: VariableDeclNode) -> str:
        indent = self._indent()
        val = self._visit(node.value) if node.value else "None"
        return f"{indent}{node.identifier} = {val}"

    def _visit_AssignmentNode(self, node: AssignmentNode) -> str:
        indent = self._indent()
        val = self._visit(node.value)
        return f"{indent}{node.identifier} = {val}"

    def _visit_OutputNode(self, node: OutputNode) -> str:
        indent = self._indent()
        val = self._visit(node.expression)
        return f"{indent}print({val})"

    def _visit_IfNode(self, node: IfNode) -> str:
        indent = self._indent()
        cond = self._visit(node.condition)
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        
        result = f"{indent}if {cond}:\n{body}"
        if node.else_body:
            self.indent_level += 1
            else_code = self._visit(node.else_body)
            self.indent_level -= 1
            result += f"\n{indent}else:\n{else_code}"
        return result

    def _visit_WhileNode(self, node: WhileNode) -> str:
        indent = self._indent()
        cond = self._visit(node.condition)
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        return f"{indent}while {cond}:\n{body}"

    def _visit_ForNode(self, node: ForNode) -> str:
        indent = self._indent()
        iterable = self._visit(node.iterable)
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        return f"{indent}for {node.iterator} in {iterable}:\n{body}"

    def _visit_FunctionDefNode(self, node: FunctionDefNode) -> str:
        indent = self._indent()
        params = ", ".join(node.parameters)
        prefix = "async def" if node.is_async else "def"
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        return f"{indent}{prefix} {node.name}({params}):\n{body}"

    def _visit_FunctionCallNode(self, node: FunctionCallNode) -> str:
        args = ", ".join(self._visit(a) for a in node.arguments)
        return f"{node.name}({args})"

    def _visit_ReturnNode(self, node: ReturnNode) -> str:
        indent = self._indent()
        if node.expression:
            return f"{indent}return {self._visit(node.expression)}"
        return f"{indent}return"

    def _visit_ImportNode(self, node: ImportNode) -> str:
        indent = self._indent()
        mod = node.module
        if mod == "ml":
            return f"{indent}import enlg.stdlib.ml as ml"
        return f"{indent}import {mod}"

    def _visit_ClassDefNode(self, node: ClassDefNode) -> str:
        indent = self._indent()
        bases = f"({', '.join(node.base_classes)})" if node.base_classes else ""
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        return f"{indent}class {node.name}{bases}:\n{body}"

    def _visit_InstantiateNode(self, node: InstantiateNode) -> str:
        args = ", ".join(self._visit(a) for a in node.arguments)
        return f"{node.class_name}({args})"

    def _visit_AttemptNode(self, node: AttemptNode) -> str:
        indent = self._indent()
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        return f"{indent}try:\n{body}"

    def _visit_RescueNode(self, node: RescueNode) -> str:
        indent = self._indent()
        self.indent_level += 1
        body = self._visit(node.body)
        self.indent_level -= 1
        err_var = f" as {node.error_name}" if node.error_name else ""
        return f"{indent}except Exception{err_var}:\n{body}"

    def _visit_RaiseNode(self, node: RaiseNode) -> str:
        indent = self._indent()
        expr = self._visit(node.expression)
        return f"{indent}raise Exception({expr})"

    def _visit_PythonInteropNode(self, node: PythonInteropNode) -> str:
        args = ", ".join(self._visit(a) for a in node.arguments)
        return f"{node.target}({args})"

    def _visit_DomainOpNode(self, node: DomainOpNode) -> str:
        indent = self._indent()
        target_name = node.target.name if isinstance(node.target, IdentifierNode) else str(node.target)
        args_str = ", ".join(self._visit(a) for a in node.arguments)

        # External module call (e.g. from numpy, from matplotlib.pyplot)
        if node.from_module:
            return f"{indent}{node.from_module}.{node.op}({args_str})"

        # Specialized ML Domain Operations
        op_lower = node.op.lower()
        if op_lower == "load":
            return f"{indent}{target_name} = ml.load_dataset({args_str})"
        elif op_lower == "preprocess":
            return f"{indent}{target_name} = ml.preprocess_dataset({target_name})"
        elif op_lower == "split":
            if node.store_result and ":" in node.store_result:
                v1, v2 = node.store_result.split(":", 1)
                return f"{indent}{v1}, {v2} = ml.split_dataset({target_name})"
            return f"{indent}train_data, test_data = ml.split_dataset({target_name})"
        elif op_lower == "train":
            return f"{indent}{target_name} = ml.train_model({target_name}, {args_str})"
        elif op_lower == "evaluate":
            return f"{indent}ml.evaluate_model({target_name}, {args_str})"

        return f"{indent}# {node.op} {target_name} {args_str}"


def transpile_enlg_source(source: str) -> str:
    """Tokenizes, parses, and transpiles .enlg code string to Python."""
    tokens = Lexer(source).tokenize()
    ast = BlockParser.parse(tokens)
    return PythonTranspiler().transpile(ast)

def transpile_enlg_file(filepath: str) -> str:
    """Reads a .enlg file and transpiles it to Python."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    return transpile_enlg_source(source)

def build_enlg_file(input_path: str, output_path: str = None) -> str:
    """Compiles/transpiles a .enlg file and writes the resulting .py file to disk."""
    import os
    py_code = transpile_enlg_file(input_path)
    if output_path is None:
        base, _ = os.path.splitext(input_path)
        output_path = f"{base}.py"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(py_code)
    print(f"[enlg] Built Python: {output_path}")
    return output_path
