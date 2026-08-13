"""enlg Expression Parser.

Parses foundational atomic expressions (Literals, Identifiers) 
and Collections (Lists, Maps) from a stream of tokens.
"""

from typing import List, Tuple
from enlg.lexer.tokens import Token, TokenType
from enlg.ast.nodes import (
    ExpressionNode, LiteralNode, IdentifierNode, 
    BooleanNode, NullNode, ListNode, MapNode, FunctionCallNode
)
from enlg.diagnostics.diagnostics import SyntaxError

class ExpressionParser:
    
    @staticmethod
    def parse(tokens: List[Token]) -> ExpressionNode:
        if not tokens:
            raise SyntaxError("E1003", "Expected expression, found nothing.")
            
        left_node, remaining = ExpressionParser._parse_single(tokens)
        
        # Check if there is an infix operator following
        if remaining:
            first_val = remaining[0].value.lower()
            
            # 1. Symbol Operators
            if remaining[0].type == TokenType.SYMBOL and first_val in (
                "+", "-", "*", "/", "%", "**", "//", "&&", "||", "&", "|", "^", "<<", ">>",
                "<", ">", "<=", ">=", "==", "!="
            ):
                op = first_val
                right_node, rest = ExpressionParser._parse_single(remaining[1:])
                if rest:
                    raise SyntaxError("E1003", f"Unexpected tokens after binary expression: {[t.value for t in rest]}")
                from enlg.ast.nodes import BinaryOpNode
                return BinaryOpNode(left=left_node, op=op, right=right_node)
                
            # 2. English operators
            elif remaining[0].type == TokenType.IDENTIFIER:
                op = None
                pos = 0
                
                if first_val in ("plus", "added"):
                    op = "+"
                    pos = 1
                elif first_val in ("minus", "subtracted"):
                    op = "-"
                    pos = 1
                elif first_val in ("times", "multiplied"):
                    op = "*"
                    pos = 1
                elif first_val == "divided":
                    op = "/"
                    pos = 1
                elif first_val in ("modulo", "mod"):
                    op = "%"
                    pos = 1
                elif first_val == "power":
                    op = "**"
                    pos = 1
                elif first_val == "integer" and len(remaining) > 1 and remaining[1].value.lower() == "divided":
                    op = "//"
                    pos = 2
                elif first_val == "and":
                    op = "&&"
                    pos = 1
                elif first_val == "or":
                    op = "||"
                    pos = 1
                elif first_val == "bitwise":
                    if len(remaining) > 1:
                        nxt = remaining[1].value.lower()
                        if nxt == "and":
                            op = "&"
                            pos = 2
                        elif nxt == "or":
                            op = "|"
                            pos = 2
                        elif nxt == "xor":
                            op = "^"
                            pos = 2
                elif first_val == "left" and len(remaining) > 1 and remaining[1].value.lower() == "shift":
                    op = "<<"
                    pos = 2
                elif first_val == "right" and len(remaining) > 1 and remaining[1].value.lower() == "shift":
                    op = ">>"
                    pos = 2
                elif first_val == "is":
                    if len(remaining) > 1 and remaining[1].value.lower() == "not":
                        op = "!="
                        pos = 2
                    else:
                        op = "=="
                        pos = 1
                elif first_val == "equals":
                    op = "=="
                    pos = 1
                elif first_val == "equal":
                    op = "=="
                    pos = 1
                elif first_val == "not":
                    if len(remaining) > 1 and remaining[1].value.lower() == "equal":
                        op = "!="
                        pos = 2
                elif first_val == "greater":
                    if len(remaining) > 1 and remaining[1].value.lower() == "than":
                        if len(remaining) > 3 and remaining[2].value.lower() == "or" and remaining[3].value.lower() == "equal":
                            pos = 4
                            op = ">="
                        else:
                            op = ">"
                            pos = 2
                elif first_val == "less":
                    if len(remaining) > 1 and remaining[1].value.lower() == "than":
                        if len(remaining) > 3 and remaining[2].value.lower() == "or" and remaining[3].value.lower() == "equal":
                            pos = 4
                            op = "<="
                        else:
                            op = "<"
                            pos = 2
                        
                if op:
                    right_node, rest = ExpressionParser._parse_single(remaining[pos:])
                    if rest:
                        raise SyntaxError("E1003", f"Unexpected tokens after comparison expression: {[t.value for t in rest]}")
                    from enlg.ast.nodes import BinaryOpNode
                    return BinaryOpNode(left=left_node, op=op, right=right_node)
                    
            raise SyntaxError("E1003", f"Unexpected tokens after expression: {[t.value for t in remaining]}")
        return left_node
        
    @staticmethod
    def _parse_single(tokens: List[Token]) -> Tuple[ExpressionNode, List[Token]]:
        token = tokens[0]
        
        # Check if it's a function call expression (e.g., "call add with 1, 2")
        from enlg.core.intents import INTENT_REGISTRY
        intent = INTENT_REGISTRY.get(token.value.lower())
        if token.type == TokenType.IDENTIFIER and intent == "FUNC_CALL":
            return ExpressionParser._parse_function_call(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "CLASS_NEW":
            return ExpressionParser._parse_instantiation(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "ASYNC_AWAIT":
            return ExpressionParser._parse_await(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "PYTHON_INTEROP":
            return ExpressionParser._parse_interop(tokens)
            
        # Check Unary Operators (-, !, ~, not, bitwise not)
        if token.type == TokenType.SYMBOL and token.value in ("-", "!", "~"):
            op = token.value
            right_node, rest = ExpressionParser._parse_single(tokens[1:])
            if op == "-" and isinstance(right_node, LiteralNode) and right_node.type_name == "number":
                return LiteralNode(value=f"-{right_node.value}", type_name="number"), rest
            from enlg.ast.nodes import UnaryOpNode
            return UnaryOpNode(op=op, operand=right_node), rest

        if token.type == TokenType.IDENTIFIER:
            val = token.value.lower()
            if val == "not":
                right_node, rest = ExpressionParser._parse_single(tokens[1:])
                from enlg.ast.nodes import UnaryOpNode
                return UnaryOpNode(op="not", operand=right_node), rest
            elif val == "bitwise" and len(tokens) > 1 and tokens[1].value.lower() == "not":
                right_node, rest = ExpressionParser._parse_single(tokens[2:])
                from enlg.ast.nodes import UnaryOpNode
                return UnaryOpNode(op="~", operand=right_node), rest
                
        # 1. Primitives & Identifiers
        if token.type == TokenType.NUMBER:
            return LiteralNode(value=token.value, type_name="number"), tokens[1:]
        elif token.type == TokenType.STRING:
            return LiteralNode(value=token.value, type_name="string"), tokens[1:]
        elif token.type == TokenType.IDENTIFIER:
            val = token.value
            if val == "true":
                return BooleanNode(value=True), tokens[1:]
            elif val == "false":
                return BooleanNode(value=False), tokens[1:]
            elif val == "null":
                return NullNode(), tokens[1:]
            else:
                return IdentifierNode(name=val), tokens[1:]
                
        # 2. Lists [...]
        elif token.type == TokenType.SYMBOL and token.value == "[":
            return ExpressionParser._parse_list(tokens)
            
        # 3. Maps {...}
        elif token.type == TokenType.SYMBOL and token.value == "{":
            return ExpressionParser._parse_map(tokens)
            
        raise SyntaxError("E1003", f"Unexpected token in expression: '{token.value}'")

    @staticmethod
    def _parse_list(tokens: List[Token]) -> Tuple[ListNode, List[Token]]:
        # tokens[0] is '['
        elements = []
        pos = 1
        
        while pos < len(tokens):
            t = tokens[pos]
            if t.type == TokenType.SYMBOL and t.value == "]":
                return ListNode(elements=elements), tokens[pos+1:]
            if t.type == TokenType.SYMBOL and t.value == ",":
                pos += 1
                continue
                
            # Grab the next element
            node, remaining = ExpressionParser._parse_single(tokens[pos:])
            elements.append(node)
            pos = len(tokens) - len(remaining)
            
        raise SyntaxError("E1002", "Unmatched '[' bracket in list expression.")

    @staticmethod
    def _parse_map(tokens: List[Token]) -> Tuple[MapNode, List[Token]]:
        # tokens[0] is '{'
        pairs = {}
        pos = 1
        
        while pos < len(tokens):
            t = tokens[pos]
            if t.type == TokenType.SYMBOL and t.value == "}":
                return MapNode(pairs=pairs), tokens[pos+1:]
            if t.type == TokenType.SYMBOL and t.value == ",":
                pos += 1
                continue
                
            # Key must be string (for now)
            key_token = tokens[pos]
            if key_token.type != TokenType.STRING:
                raise SyntaxError("E1003", f"Map keys must be strings, found: {key_token.value}")
            key = key_token.value
            pos += 1
            
            # Colon ':'
            if pos >= len(tokens) or not (tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ":"):
                raise SyntaxError("E1003", f"Expected ':' after map key '{key}'")
            pos += 1
            
            # Value
            val_node, remaining = ExpressionParser._parse_single(tokens[pos:])
            pairs[key] = val_node
            pos = len(tokens) - len(remaining)
            
        raise SyntaxError("E1002", "Unmatched '{' brace in map expression.")

    @staticmethod
    def _parse_function_call(tokens: List[Token]) -> Tuple[FunctionCallNode, List[Token]]:
        # tokens[0] is the call keyword ('call', 'invoke', etc)
        if len(tokens) < 2 or tokens[1].type != TokenType.IDENTIFIER:
            raise SyntaxError("E1003", "Missing function name in call expression.")
            
        func_name = tokens[1].value
        pos = 2
        args = []
        
        from enlg.core.intents import CONNECTORS
        
        while pos < len(tokens):
            t_val = tokens[pos].value.lower() if tokens[pos].type == TokenType.IDENTIFIER else ""
            
            # 1. Check for 'from <module>' or 'using <module>' module scoping clause
            if t_val in ("from", "using"):
                if pos + 1 < len(tokens) and tokens[pos+1].type == TokenType.IDENTIFIER:
                    mod_name = tokens[pos+1].value
                    if "." not in func_name:
                        func_name = f"{mod_name}.{func_name}"
                    pos += 2
                    continue
                    
            # 2. Skip filler connectors ("with", "using", "to", etc.) and commas
            if (tokens[pos].type == TokenType.IDENTIFIER and t_val in CONNECTORS) or (tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ","):
                pos += 1
                continue
                
            try:
                arg_node, remaining = ExpressionParser._parse_single(tokens[pos:])
                args.append(arg_node)
                pos = len(tokens) - len(remaining)
            except SyntaxError:
                break
                
        return FunctionCallNode(name=func_name, arguments=args), tokens[pos:]

    @staticmethod
    def _parse_instantiation(tokens: List[Token]) -> Tuple['InstantiateNode', List[Token]]:
        from enlg.ast.nodes import InstantiateNode
        # tokens[0] is 'new' or 'instantiate'
        if len(tokens) < 2 or tokens[1].type != TokenType.IDENTIFIER:
            raise SyntaxError("E1003", "Missing class name in instantiation.")
            
        class_name = tokens[1].value
        pos = 2
        args = []
        
        # Skip connectors like "with", "using"
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens) and tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() in CONNECTORS:
            pos += 1
            
        while pos < len(tokens):
            if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ",":
                pos += 1
                continue
                
            try:
                arg_node, remaining = ExpressionParser._parse_single(tokens[pos:])
                args.append(arg_node)
                pos = len(tokens) - len(remaining)
            except SyntaxError:
                break
                
        return InstantiateNode(class_name=class_name, arguments=args), tokens[pos:]

    @staticmethod
    def _parse_await(tokens: List[Token]) -> Tuple['AwaitNode', List[Token]]:
        from enlg.ast.nodes import AwaitNode
        if len(tokens) < 2:
            raise SyntaxError("E1003", "Missing expression to await.")
            
        pos = 1
        # skip connectors like "for" if user typed "wait for"
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens) and tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() in CONNECTORS:
            pos += 1
            
        expr_node, remaining = ExpressionParser._parse_single(tokens[pos:])
        return AwaitNode(expression=expr_node), remaining

    @staticmethod
    def _parse_interop(tokens: List[Token]) -> Tuple['PythonInteropNode', List[Token]]:
        from enlg.ast.nodes import PythonInteropNode
        if len(tokens) < 2:
            raise SyntaxError("E1003", "Missing target for python interop.")
            
        # Target could be an identifier or a string, let's treat identifier mapping to python paths.
        target = tokens[1].value
        pos = 2
        args = []
        
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens) and tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() in CONNECTORS:
            pos += 1
            
        while pos < len(tokens):
            if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ",":
                pos += 1
                continue
            try:
                arg_node, remaining = ExpressionParser._parse_single(tokens[pos:])
                args.append(arg_node)
                pos = len(tokens) - len(remaining)
            except SyntaxError:
                break
                
        return PythonInteropNode(target=target, arguments=args), tokens[pos:]
