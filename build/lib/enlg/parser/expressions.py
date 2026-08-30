"""enlg Expression Parser - Complete Pratt Parser Edition.

Parses atomic expressions, grouping with parentheses, unary operators,
binary operators with mathematical precedence, indexing, list/map collections,
and intent-based calls/instantiations.
"""

from typing import List, Tuple, Optional
from enlg.lexer.tokens import Token, TokenType
from enlg.ast.nodes import (
    ExpressionNode, LiteralNode, IdentifierNode, 
    BooleanNode, NullNode, ListNode, MapNode, FunctionCallNode,
    BinaryOpNode, UnaryOpNode
)
from enlg.diagnostics.diagnostics import SyntaxError

# Precedence table (higher number = tighter binding)
PRECEDENCE_MAP = {
    "or": 1, "||": 1,
    "and": 2, "&&": 2,
    "==": 3, "!=": 3, "<": 3, "<=": 3, ">": 3, ">=": 3,
    "is": 3, "is not": 3, "equals": 3, "equal": 3, "not equal": 3,
    "greater than": 3, "less than": 3, "greater than or equal to": 3, "less than or equal to": 3,
    "|": 4, "^": 4,
    "&": 5,
    "<<": 6, ">>": 6,
    "+": 7, "-": 7, "plus": 7, "minus": 7,
    "*": 8, "/": 8, "%": 8, "//": 8, "times": 8, "divided": 8, "mod": 8, "modulo": 8,
    "**": 9, "power": 9
}

class ExpressionParser:

    @staticmethod
    def parse(tokens: List[Token]) -> ExpressionNode:
        """Parses a token stream into an ExpressionNode tree using Pratt parsing."""
        if not tokens:
            raise SyntaxError("E1003", "Expected expression, found nothing.")
            
        node, remaining = ExpressionParser._parse_pratt(tokens, min_prec=0)
        if remaining:
            raise SyntaxError("E1003", f"Unexpected tokens after expression: {[t.value for t in remaining]}")
        return node

    @staticmethod
    def _match_binary_op(tokens: List[Token]) -> Tuple[Optional[str], Optional[int], int]:
        """
        Attempts to match an infix binary operator from the start of tokens.
        Returns (canonical_op_str, precedence, tokens_consumed_count).
        If no operator matches, returns (None, None, 0).
        """
        if not tokens:
            return None, None, 0

        t0 = tokens[0]
        val0 = t0.value.lower()

        # 1. Symbol Operators
        if t0.type == TokenType.SYMBOL and val0 in PRECEDENCE_MAP:
            return val0, PRECEDENCE_MAP[val0], 1

        # 2. Multi-word and single-word English Operators
        if t0.type == TokenType.IDENTIFIER:
            # Check 4-word operators: "greater than or equal", "less than or equal"
            if len(tokens) >= 4:
                w4 = " ".join([tokens[i].value.lower() for i in range(4)])
                if w4 in ("greater than or equal to", "greater than or equal"):
                    return ">=", PRECEDENCE_MAP[">="], 4
                elif w4 in ("less than or equal to", "less than or equal"):
                    return "<=", PRECEDENCE_MAP["<="], 4

            # Check 2-word operators
            if len(tokens) >= 2:
                w2 = f"{val0} {tokens[1].value.lower()}"
                if w2 in ("is not", "not equal"):
                    return "!=", PRECEDENCE_MAP["!="], 2
                elif w2 == "greater than":
                    return ">", PRECEDENCE_MAP[">"], 2
                elif w2 == "less than":
                    return "<", PRECEDENCE_MAP["<"], 2
                elif w2 == "integer divided":
                    return "//", PRECEDENCE_MAP["//"], 2
                elif w2 == "left shift":
                    return "<<", PRECEDENCE_MAP["<<"], 2
                elif w2 == "right shift":
                    return ">>", PRECEDENCE_MAP[">>"], 2
                elif w2 == "bitwise and":
                    return "&", PRECEDENCE_MAP["&"], 2
                elif w2 == "bitwise or":
                    return "|", PRECEDENCE_MAP["|"], 2
                elif w2 == "bitwise xor":
                    return "^", PRECEDENCE_MAP["^"], 2

            # Check 1-word English operators
            if val0 in ("plus", "added"):
                return "+", PRECEDENCE_MAP["+"], 1
            elif val0 in ("minus", "subtracted"):
                return "-", PRECEDENCE_MAP["-"], 1
            elif val0 in ("times", "multiplied"):
                return "*", PRECEDENCE_MAP["*"], 1
            elif val0 == "divided":
                return "/", PRECEDENCE_MAP["/"], 1
            elif val0 in ("modulo", "mod"):
                return "%", PRECEDENCE_MAP["%"], 1
            elif val0 == "power":
                return "**", PRECEDENCE_MAP["**"], 1
            elif val0 in ("and",):
                return "&&", PRECEDENCE_MAP["and"], 1
            elif val0 in ("or",):
                return "||", PRECEDENCE_MAP["or"], 1
            elif val0 in ("is", "equals", "equal"):
                return "==", PRECEDENCE_MAP["=="], 1

        return None, None, 0

    @staticmethod
    def _parse_pratt(tokens: List[Token], min_prec: int = 0) -> Tuple[ExpressionNode, List[Token]]:
        """Precedence Climbing Pratt Parser."""
        if not tokens:
            raise SyntaxError("E1003", "Expected expression, found nothing.")

        # 1. Parse Prefix / Atom
        left_node, remaining = ExpressionParser._parse_prefix(tokens)

        # 2. Infix / Postfix Loop
        while remaining:
            # Check Postfix Indexing `arr[idx]`
            if remaining[0].type == TokenType.SYMBOL and remaining[0].value == "[":
                idx_tokens = remaining[1:]
                bracket_level = 1
                pos = 0
                while pos < len(idx_tokens):
                    if idx_tokens[pos].type == TokenType.SYMBOL and idx_tokens[pos].value == "[":
                        bracket_level += 1
                    elif idx_tokens[pos].type == TokenType.SYMBOL and idx_tokens[pos].value == "]":
                        bracket_level -= 1
                        if bracket_level == 0:
                            break
                    pos += 1
                if pos >= len(idx_tokens):
                    raise SyntaxError("E1002", "Unmatched '[' bracket in index expression.")

                idx_expr = ExpressionParser.parse(idx_tokens[:pos])
                left_node = BinaryOpNode(left=left_node, op="[]", right=idx_expr)
                remaining = idx_tokens[pos+1:]
                continue

            # Check Infix Binary Operator
            op_sym, op_prec, consumed = ExpressionParser._match_binary_op(remaining)
            if op_sym is None or op_prec < min_prec:
                break

            # Right associativity for power '**'
            next_prec = op_prec if op_sym == "**" else op_prec + 1
            right_node, remaining = ExpressionParser._parse_pratt(remaining[consumed:], min_prec=next_prec)
            left_node = BinaryOpNode(left=left_node, op=op_sym, right=right_node)

        return left_node, remaining

    @staticmethod
    def _parse_prefix(tokens: List[Token]) -> Tuple[ExpressionNode, List[Token]]:
        """Parses prefix unary, grouping parentheses, atoms, and keyword intents."""
        token = tokens[0]
        val_lower = token.value.lower()

        # 1. Grouping Parentheses: `( expr )`
        if token.type == TokenType.SYMBOL and token.value == "(":
            paren_level = 1
            pos = 1
            while pos < len(tokens):
                if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == "(":
                    paren_level += 1
                elif tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ")":
                    paren_level -= 1
                    if paren_level == 0:
                        break
                pos += 1
            if pos >= len(tokens):
                raise SyntaxError("E1002", "Unmatched '(' in expression.")

            inner_expr = ExpressionParser.parse(tokens[1:pos])
            return inner_expr, tokens[pos+1:]

        # 2. Intent Expressions (call, new, await, native)
        from enlg.core.intents import INTENT_REGISTRY
        intent = INTENT_REGISTRY.get(val_lower)
        if token.type == TokenType.IDENTIFIER and intent == "FUNC_CALL":
            return ExpressionParser._parse_function_call(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "CLASS_NEW":
            return ExpressionParser._parse_instantiation(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "ASYNC_AWAIT":
            return ExpressionParser._parse_await(tokens)
        elif token.type == TokenType.IDENTIFIER and intent == "PYTHON_INTEROP":
            return ExpressionParser._parse_interop(tokens)

        # 3. Unary Operators (-, !, ~, not)
        if token.type == TokenType.SYMBOL and token.value in ("-", "!", "~"):
            op = token.value
            operand_node, rest = ExpressionParser._parse_pratt(tokens[1:], min_prec=8)
            if op == "-" and isinstance(operand_node, LiteralNode) and operand_node.type_name == "number":
                return LiteralNode(value=f"-{operand_node.value}", type_name="number"), rest
            return UnaryOpNode(op=op, operand=operand_node), rest

        if token.type == TokenType.IDENTIFIER and val_lower == "not":
            operand_node, rest = ExpressionParser._parse_pratt(tokens[1:], min_prec=3)
            return UnaryOpNode(op="not", operand=operand_node), rest

        # 4. Primitives & Literals
        if token.type == TokenType.NUMBER:
            return LiteralNode(value=token.value, type_name="number"), tokens[1:]
        elif token.type == TokenType.STRING:
            return LiteralNode(value=token.value, type_name="string"), tokens[1:]
        elif token.type == TokenType.IDENTIFIER:
            if val_lower == "true":
                return BooleanNode(value=True), tokens[1:]
            elif val_lower == "false":
                return BooleanNode(value=False), tokens[1:]
            elif val_lower == "null":
                return NullNode(), tokens[1:]
            else:
                return IdentifierNode(name=token.value), tokens[1:]

        # 5. Lists [...]
        elif token.type == TokenType.SYMBOL and token.value == "[":
            return ExpressionParser._parse_list(tokens)

        # 6. Maps {...}
        elif token.type == TokenType.SYMBOL and token.value == "{":
            return ExpressionParser._parse_map(tokens)

        raise SyntaxError("E1003", f"Unexpected token in expression: '{token.value}'")

    @staticmethod
    def _parse_single(tokens: List[Token]) -> Tuple[ExpressionNode, List[Token]]:
        """Legacy helper for single atom/prefix expression parsing."""
        return ExpressionParser._parse_prefix(tokens)

    @staticmethod
    def _parse_list(tokens: List[Token]) -> Tuple[ListNode, List[Token]]:
        elements = []
        pos = 1
        
        while pos < len(tokens):
            t = tokens[pos]
            if t.type == TokenType.SYMBOL and t.value == "]":
                return ListNode(elements=elements), tokens[pos+1:]
            if t.type == TokenType.SYMBOL and t.value == ",":
                pos += 1
                continue
                
            node, remaining = ExpressionParser._parse_pratt(tokens[pos:], min_prec=0)
            elements.append(node)
            pos = len(tokens) - len(remaining)
            
        raise SyntaxError("E1002", "Unmatched '[' bracket in list expression.")

    @staticmethod
    def _parse_map(tokens: List[Token]) -> Tuple[MapNode, List[Token]]:
        pairs = {}
        pos = 1
        
        while pos < len(tokens):
            t = tokens[pos]
            if t.type == TokenType.SYMBOL and t.value == "}":
                return MapNode(pairs=pairs), tokens[pos+1:]
            if t.type == TokenType.SYMBOL and t.value == ",":
                pos += 1
                continue
                
            key_token = tokens[pos]
            if key_token.type != TokenType.STRING:
                raise SyntaxError("E1003", f"Map keys must be strings, found: {key_token.value}")
            key = key_token.value
            pos += 1
            
            if pos >= len(tokens) or not (tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ":"):
                raise SyntaxError("E1003", f"Expected ':' after map key '{key}'")
            pos += 1
            
            val_node, remaining = ExpressionParser._parse_pratt(tokens[pos:], min_prec=0)
            pairs[key] = val_node
            pos = len(tokens) - len(remaining)
            
        raise SyntaxError("E1002", "Unmatched '{' brace in map expression.")

    @staticmethod
    def _parse_function_call(tokens: List[Token]) -> Tuple[FunctionCallNode, List[Token]]:
        if len(tokens) < 2 or tokens[1].type != TokenType.IDENTIFIER:
            raise SyntaxError("E1003", "Missing function name in call expression.")
            
        func_name = tokens[1].value
        pos = 2
        args = []
        
        from enlg.core.intents import CONNECTORS
        
        while pos < len(tokens):
            t_val = tokens[pos].value.lower() if tokens[pos].type == TokenType.IDENTIFIER else ""
            
            if t_val in ("from", "using"):
                if pos + 1 < len(tokens) and tokens[pos+1].type == TokenType.IDENTIFIER:
                    mod_name = tokens[pos+1].value
                    if "." not in func_name:
                        func_name = f"{mod_name}.{func_name}"
                    pos += 2
                    continue

            if len(args) > 0:
                # Subsequent arguments MUST be preceded by a comma or connector
                if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ",":
                    pos += 1
                elif tokens[pos].type == TokenType.IDENTIFIER and t_val in CONNECTORS:
                    pos += 1
                else:
                    # Argument list has ended; remaining tokens belong to outer expression (e.g. '- 1')
                    break
            else:
                # Skip leading connectors for first argument ('with', 'using', 'to', etc.)
                if tokens[pos].type == TokenType.IDENTIFIER and t_val in CONNECTORS:
                    pos += 1
                    continue
                
            if pos >= len(tokens):
                break

            try:
                arg_node, remaining = ExpressionParser._parse_prefix(tokens[pos:])
                args.append(arg_node)
                pos = len(tokens) - len(remaining)
            except SyntaxError:
                break
                
        return FunctionCallNode(name=func_name, arguments=args), tokens[pos:]

    @staticmethod
    def _parse_instantiation(tokens: List[Token]) -> Tuple['InstantiateNode', List[Token]]:
        from enlg.ast.nodes import InstantiateNode
        if len(tokens) < 2 or tokens[1].type != TokenType.IDENTIFIER:
            raise SyntaxError("E1003", "Missing class name in instantiation.")
            
        class_name = tokens[1].value
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
                arg_node, remaining = ExpressionParser._parse_prefix(tokens[pos:])
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
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens) and tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() in CONNECTORS:
            pos += 1
            
        expr_node, remaining = ExpressionParser._parse_prefix(tokens[pos:])
        return AwaitNode(expression=expr_node), remaining

    @staticmethod
    def _parse_interop(tokens: List[Token]) -> Tuple['PythonInteropNode', List[Token]]:
        from enlg.ast.nodes import PythonInteropNode
        if len(tokens) < 2:
            raise SyntaxError("E1003", "Missing target for python interop.")
            
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
                arg_node, remaining = ExpressionParser._parse_prefix(tokens[pos:])
                args.append(arg_node)
                pos = len(tokens) - len(remaining)
            except SyntaxError:
                break
                
        return PythonInteropNode(target=target, arguments=args), tokens[pos:]
