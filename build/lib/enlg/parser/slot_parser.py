"""enlg Flexible Slot Parser.

Transforms LockedStatements into explicitly typed AST Nodes by extracting
operands flexibly. Adheres to fail-closed mechanics if slots are missing.
"""

from typing import List, Optional
from enlg.lexer.tokens import Token, TokenType
from enlg.parser.discovery import LockedStatement
from enlg.ast.nodes import (
    StatementNode, VariableDeclNode, AssignmentNode, OutputNode
)
from enlg.parser.expressions import ExpressionParser
from enlg.diagnostics.diagnostics import SyntaxError

class SlotParser:
    """Extracts operands from a LockedStatement into an AST node."""
    
    @staticmethod
    def parse(statement: LockedStatement) -> StatementNode:
        return SlotParser.parse_with_body(statement, None)

    @staticmethod
    def parse_with_body(statement: LockedStatement, body: Optional['BlockNode']) -> StatementNode:
        from enlg.ast.nodes import BlockNode, IfNode, WhileNode
        intent = statement.intent_id
        tokens = statement.remaining_tokens
        
        if intent == "DECLARE_VARIABLE":
            return SlotParser._parse_declaration(tokens)
        elif intent in ("ASSIGN_VARIABLE", "ASSIGN_COMPOUND_ADD", "ASSIGN_COMPOUND_SUB", "ASSIGN_COMPOUND_MUL", "ASSIGN_COMPOUND_DIV"):
            return SlotParser._parse_assignment(tokens, intent)
        elif intent == "OUTPUT_DISPLAY":
            return SlotParser._parse_output(tokens)
        elif intent in ("COND_IF", "COND_ELIF"):
            if not body:
                raise SyntaxError("E1003", "If/Else If statement requires an indented block.")
            return SlotParser._parse_if(tokens, body)
        elif intent == "COND_ELSE":
            if not body:
                raise SyntaxError("E1003", "Else statement requires an indented block.")
            from enlg.ast.nodes import BooleanNode, IfNode
            return IfNode(condition=BooleanNode(True), body=body)
        elif intent in ("LOOP_WHILE", "LOOP_REPEAT"):
            if not body:
                raise SyntaxError("E1003", "Loop statement requires an indented block.")
            return SlotParser._parse_while(tokens, body)
        elif intent == "FUNC_DEF":
            if not body:
                raise SyntaxError("E1003", "Function definition requires an indented block.")
            return SlotParser._parse_function_def(tokens, body)
        elif intent == "FUNC_RETURN":
            return SlotParser._parse_return(tokens)
        elif intent == "BLOCK_TRY":
            if not body:
                raise SyntaxError("E1003", "Attempt/try block requires an indented block.")
            from enlg.ast.nodes import AttemptNode
            return AttemptNode(body=body)
        elif intent == "BLOCK_CATCH":
            if not body:
                raise SyntaxError("E1003", "Rescue/catch block requires an indented block.")
            return SlotParser._parse_rescue(tokens, body)
        elif intent == "STMT_RAISE":
            return SlotParser._parse_raise(tokens)
        elif intent == "STMT_IMPORT":
            return SlotParser._parse_import(tokens)
        elif intent in (
            "AI_TRAIN", "AI_PREDICT", "AI_EVALUATE",
            "AI_LOAD", "AI_PREPROCESS", "AI_SPLIT",
            "AI_SAVE", "AI_RESTORE", "AI_FIT",
            "DL_COMPILE", "DL_FORWARD",
            "SEC_SCAN", "SEC_ENCRYPT",
            "CLOUD_DEPLOY", "CLOUD_FETCH",
        ):
            return SlotParser._parse_domain_op(intent, tokens)
        elif intent == "CLASS_DEF":
            if not body:
                raise SyntaxError("E1003", "Class definition requires an indented block.")
            return SlotParser._parse_class_def(tokens, body)
        elif intent in ("CLASS_NEW", "FUNC_CALL", "ASYNC_AWAIT", "PYTHON_INTEROP"):
            return SlotParser._parse_expression_statement(statement)
            
        raise SyntaxError("E1003", f"Unsupported intent for slot parsing: {intent}")

    @staticmethod
    def _parse_class_def(tokens: List[Token], body: 'BlockNode') -> 'StatementNode':
        from enlg.ast.nodes import ClassDefNode
        if not tokens:
            raise SyntaxError("E1003", "Missing class name.")
            
        name = tokens[0].value
        base_classes = []
        pos = 1
        
        # Check for inheritance keywords
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens):
            val = tokens[pos].value.lower()
            if val in ("inherits", "extends"):
                pos += 1
                while pos < len(tokens):
                    if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ",":
                        pos += 1
                        continue
                    if tokens[pos].type == TokenType.IDENTIFIER:
                        base_classes.append(tokens[pos].value)
                    else:
                        raise SyntaxError("E1003", f"Invalid base class name: {tokens[pos].value}")
                    pos += 1
                break
            pos += 1
            
        return ClassDefNode(name=name, base_classes=base_classes, body=body)

    @staticmethod
    def _parse_expression_statement(statement: LockedStatement) -> 'StatementNode':
        full_tokens = [statement.hint_token] + statement.remaining_tokens
        expr_node = ExpressionParser.parse(full_tokens)
        return expr_node

    @staticmethod
    def _parse_rescue(tokens: List[Token], body: 'BlockNode') -> 'StatementNode':
        from enlg.ast.nodes import RescueNode
        if not tokens:
            raise SyntaxError("E1003", "Missing error variable in rescue block.")
        # Just grab the first identifier as the error name
        for t in tokens:
            if t.type == TokenType.IDENTIFIER:
                return RescueNode(error_name=t.value, body=body)
        raise SyntaxError("E1003", "Could not find error identifier in rescue statement.")

    @staticmethod
    def _parse_raise(tokens: List[Token]) -> 'StatementNode':
        from enlg.ast.nodes import RaiseNode
        if not tokens:
            raise SyntaxError("E1003", "Missing expression to throw.")
        expr_node = ExpressionParser.parse(tokens)
        return RaiseNode(expression=expr_node)

    @staticmethod
    def _parse_import(tokens: List[Token]) -> 'StatementNode':
        from enlg.ast.nodes import ImportNode
        if not tokens:
            raise SyntaxError("E1003", "Invalid module name in import statement.")
            
        module_name = tokens[0].value
        return ImportNode(module=module_name, aliases=[])

    @staticmethod
    def _parse_function_def(tokens: List[Token], body: 'BlockNode') -> 'StatementNode':
        from enlg.ast.nodes import FunctionDefNode
        if not tokens:
            raise SyntaxError("E1003", "Missing function name.")
        
        is_async = False
        pos = 0
        if tokens[0].type == TokenType.IDENTIFIER and tokens[0].value.lower() == "async":
            is_async = True
            pos = 1
            
        if pos >= len(tokens):
            raise SyntaxError("E1003", "Missing function name after async modifier.")
            
        name = tokens[pos].value
        params = []
        pos += 1
        
        # skip connectors
        from enlg.core.intents import CONNECTORS
        while pos < len(tokens) and tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() in CONNECTORS:
            pos += 1
            
        while pos < len(tokens):
            if tokens[pos].type == TokenType.SYMBOL and tokens[pos].value == ",":
                pos += 1
                continue
            if tokens[pos].type == TokenType.IDENTIFIER:
                params.append(tokens[pos].value)
            else:
                raise SyntaxError("E1003", f"Invalid parameter name: {tokens[pos].value}")
            pos += 1
            
        return FunctionDefNode(name=name, parameters=params, body=body, is_async=is_async)

    @staticmethod
    def _parse_return(tokens: List[Token]) -> 'StatementNode':
        from enlg.ast.nodes import ReturnNode
        if not tokens:
            return ReturnNode(expression=None)
        expr_node = ExpressionParser.parse(tokens)
        return ReturnNode(expression=expr_node)

    @staticmethod
    def _parse_if(tokens: List[Token], body: 'BlockNode') -> 'IfNode':
        from enlg.ast.nodes import IfNode
        if not tokens:
            raise SyntaxError("E1003", "Missing condition in if statement.")
        # Simplified for Phase 5: we parse the condition as a single atomic expression.
        # Future phases will add boolean operators (==, <, >).
        expr_node = ExpressionParser.parse(tokens)
        return IfNode(condition=expr_node, body=body)

    @staticmethod
    def _parse_while(tokens: List[Token], body: 'BlockNode') -> 'WhileNode':
        from enlg.ast.nodes import WhileNode
        if not tokens:
            raise SyntaxError("E1003", "Missing condition in while loop.")
        expr_node = ExpressionParser.parse(tokens)
        return WhileNode(condition=expr_node, body=body)

    @staticmethod
    def _parse_declaration(tokens: List[Token]) -> VariableDeclNode:
        identifier: Optional[str] = None
        expr_tokens: List[Token] = []
        
        # Flexibly find the identifier and the expression part
        for t in tokens:
            if t.type == TokenType.SYMBOL and t.value == "=":
                continue # Skip explicit equals
                
            if identifier is None and t.type == TokenType.IDENTIFIER:
                identifier = t.value
            else:
                expr_tokens.append(t)
                
        if identifier is None:
            raise SyntaxError("E1003", "Missing identifier in variable declaration.")
            
        expr_node = None
        if expr_tokens:
            expr_node = ExpressionParser.parse(expr_tokens)
            
        return VariableDeclNode(identifier=identifier, value=expr_node)

    @staticmethod
    def _parse_assignment(tokens: List[Token], intent: str = "ASSIGN_VARIABLE") -> AssignmentNode:
        from enlg.ast.nodes import IdentifierNode, BinaryOpNode
        
        compound_ops = {
            "+=": "+", "-=": "-", "*=": "*", "/=": "/", "%=": "%",
            "**=": "**", "//=": "//", "&=": "&", "|=": "|", "^=": "^",
            "<<=": "<<", ">>=": ">>"
        }
        
        intent_ops = {
            "ASSIGN_COMPOUND_ADD": "+",
            "ASSIGN_COMPOUND_SUB": "-",
            "ASSIGN_COMPOUND_MUL": "*",
            "ASSIGN_COMPOUND_DIV": "/"
        }
        
        identifier: Optional[str] = None
        expr_tokens: List[Token] = []
        found_compound_op: Optional[str] = intent_ops.get(intent)
        
        for t in tokens:
            if t.type == TokenType.SYMBOL and t.value == "=":
                continue
            elif t.type == TokenType.SYMBOL and t.value in compound_ops:
                found_compound_op = compound_ops[t.value]
                continue
                
            if identifier is None and t.type == TokenType.IDENTIFIER:
                identifier = t.value
            else:
                expr_tokens.append(t)
                
        if identifier is None:
            raise SyntaxError("E1003", "Missing target identifier in assignment.")
        if not expr_tokens:
            raise SyntaxError("E1003", "Missing value in assignment.")
            
        rhs_node = ExpressionParser.parse(expr_tokens)
        if found_compound_op:
            final_val = BinaryOpNode(left=IdentifierNode(name=identifier), op=found_compound_op, right=rhs_node)
        else:
            final_val = rhs_node
            
        return AssignmentNode(identifier=identifier, value=final_val)

    @staticmethod
    def _parse_output(tokens: List[Token]) -> OutputNode:
        if not tokens:
            raise SyntaxError("E1003", "Missing expression for output statement.")
            
        expr_node = ExpressionParser.parse(tokens)
        return OutputNode(expression=expr_node)

    @staticmethod
    def _parse_domain_op(intent: str, tokens: List[Token]) -> 'DomainOpNode':
        from enlg.ast.nodes import DomainOpNode, IdentifierNode
        from enlg.core.intents import CONNECTORS
        if not tokens:
            raise SyntaxError("E1003", f"Missing parameters for domain operation '{intent}'")

        # Extract 'from <module>' scoping (Only 'from', not 'using', to avoid breaking 'predict X using Y')
        from_module = None
        filtered_tokens = []
        pos = 0
        while pos < len(tokens):
            if tokens[pos].type == TokenType.IDENTIFIER and tokens[pos].value.lower() == "from":
                if pos + 1 < len(tokens) and tokens[pos+1].type == TokenType.IDENTIFIER:
                    from_module = tokens[pos+1].value
                    pos += 2
                    continue
            filtered_tokens.append(tokens[pos])
            pos += 1
            
        tokens = filtered_tokens

        # ── Auto-store intents: "load X from Y", "restore X from Y"
        # First token is the DESTINATION var name (doesn't exist yet → LOAD_CONST)
        AUTO_STORE_INTENTS = {"AI_LOAD", "AI_RESTORE"}

        # ── Store-back intents: var EXISTS in env, result stored back into same var
        # AI_PREPROCESS: "preprocess email_data" → LOAD_VAR → preprocess → STORE_VAR
        # AI_TRAIN:      "train model with data" → LOAD_VAR → train → STORE_VAR
        # AI_FIT:        "fit net with data"     → LOAD_VAR → fit   → STORE_VAR
        STORE_BACK_INTENTS = {"AI_TRAIN", "AI_FIT", "AI_PREPROCESS"}
        if intent in STORE_BACK_INTENTS and tokens[0].type == TokenType.IDENTIFIER:
            model_var = tokens[0].value
            model_node = IdentifierNode(name=model_var)
            rest = tokens[1:]
            filtered = [t for t in rest
                        if not (t.type == TokenType.IDENTIFIER and t.value.lower() in CONNECTORS)]
            args = []
            pos = 0
            while pos < len(filtered):
                if filtered[pos].type == TokenType.SYMBOL and filtered[pos].value == ",":
                    pos += 1
                    continue
                try:
                    arg_node, rest2 = ExpressionParser._parse_single(filtered[pos:])
                    args.append(arg_node)
                    consumed = len(filtered[pos:]) - len(rest2)
                    pos += consumed if consumed > 0 else 1
                except Exception:
                    break
            return DomainOpNode(op=intent, target=model_node, arguments=args,
                                store_result=model_var, store_back=True, from_module=from_module)

        store_result = None

        if intent == "AI_SPLIT" and tokens[0].type == TokenType.IDENTIFIER:
            # Pattern: split <source_var> into <train_var> and <test_var>
            source_var = tokens[0].value
            source_node = IdentifierNode(name=source_var)
            remaining_after_source = tokens[1:]
            # Collect identifier tokens after filtering connectors (into, and)
            dest_vars = [t.value for t in remaining_after_source
                         if t.type == TokenType.IDENTIFIER and t.value.lower() not in CONNECTORS]
            # Also collect any numeric ratio tokens
            ratio_args = [t for t in remaining_after_source
                          if t.type == TokenType.NUMBER]
            args = [IdentifierNode(name=v) for v in dest_vars]
            # store_result encodes "train_var:test_var" so generator knows both names
            store_result = ":".join(dest_vars) if dest_vars else source_var
            return DomainOpNode(op=intent, target=source_node, arguments=args, store_result=store_result, from_module=from_module)

        if intent in AUTO_STORE_INTENTS and tokens[0].type == TokenType.IDENTIFIER:
            # Variable name to store result into
            store_result = tokens[0].value
            remaining_after_var = tokens[1:]
            # Filter connectors
            filtered = [t for t in remaining_after_var
                        if not (t.type == TokenType.IDENTIFIER and t.value.lower() in CONNECTORS)]
            args = []
            pos = 0
            while pos < len(filtered):
                if filtered[pos].type == TokenType.SYMBOL and filtered[pos].value == ",":
                    pos += 1
                    continue
                try:
                    arg_node, rest = ExpressionParser._parse_single(filtered[pos:])
                    args.append(arg_node)
                    consumed = len(filtered[pos:]) - len(rest)
                    pos += consumed if consumed > 0 else 1
                except Exception:
                    break
            # target is a dummy identifier (the var itself)
            target_node = IdentifierNode(name=store_result)
            return DomainOpNode(op=intent, target=target_node, arguments=args, store_result=store_result, from_module=from_module)

        # Standard ops: target is first expression, rest are args
        target_node, remaining = ExpressionParser._parse_single(tokens)

        # Filter connectors from remaining tokens
        filtered_remaining = [t for t in remaining if not (t.type == TokenType.IDENTIFIER and t.value.lower() in CONNECTORS)]

        args = []
        pos = 0
        while pos < len(filtered_remaining):
            if filtered_remaining[pos].type == TokenType.SYMBOL and filtered_remaining[pos].value == ",":
                pos += 1
                continue
            try:
                arg_node, rest = ExpressionParser._parse_single(filtered_remaining[pos:])
                args.append(arg_node)
                pos = len(filtered_remaining) - len(rest)
            except SyntaxError:
                break
                
        return DomainOpNode(op=intent, target=target_node, arguments=args, from_module=from_module)
