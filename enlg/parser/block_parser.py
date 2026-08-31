"""enlg Block Parser.

Handles block indentation scoping and statement grouping for
control flow structures like if, while, and for loops.
"""

from typing import List, Tuple
from enlg.lexer.tokens import Token, TokenType
from enlg.ast.nodes import BlockNode, StatementNode, IfNode, WhileNode
from enlg.parser.discovery import IntentDiscoveryEngine
from enlg.parser.slot_parser import SlotParser
from enlg.diagnostics.diagnostics import SyntaxError

class BlockParser:
    
    @staticmethod
    def parse(tokens: List[Token]) -> BlockNode:
        """Parses a sequence of tokens into a BlockNode."""
        statements = []
        pos = 0
        
        while pos < len(tokens):
            if tokens[pos].type in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT):
                pos += 1
                continue
                
            # Extract one line of tokens
            line_tokens = []
            while pos < len(tokens) and tokens[pos].type not in (TokenType.NEWLINE, TokenType.EOF):
                line_tokens.append(tokens[pos])
                pos += 1
                
            if not line_tokens:
                continue
                
            expects_block = False
            if line_tokens[-1].type == TokenType.SYMBOL and line_tokens[-1].value == ":":
                expects_block = True
                line_tokens = line_tokens[:-1] # Remove colon

            # Check for standard domain header: "type enlg" or "type core"
            if len(line_tokens) >= 2 and line_tokens[0].value.lower() == "type" and line_tokens[1].value.lower() in ("enlg", "core", "logic", "script"):
                continue

            # Lock Intent
            locked_stmt = IntentDiscoveryEngine.process_statement(line_tokens)
            
            if expects_block:
                # The next tokens should be an INDENT, then a block, then a DEDENT
                pos += 1 # skip NEWLINE
                if pos >= len(tokens) or tokens[pos].type != TokenType.INDENT:
                    raise SyntaxError("E1002", "Expected an indented block after ':'.")
                pos += 1 # skip INDENT
                
                # Extract block tokens until the matching DEDENT
                block_tokens = []
                indent_level = 1
                while pos < len(tokens):
                    t = tokens[pos]
                    if t.type == TokenType.INDENT:
                        indent_level += 1
                    elif t.type == TokenType.DEDENT:
                        indent_level -= 1
                        if indent_level == 0:
                            pos += 1 # consume matching DEDENT
                            break
                    block_tokens.append(t)
                    pos += 1
                    
                if indent_level > 0:
                    raise SyntaxError("E1002", "Unexpected EOF while parsing indented block.")
                    
                body_block = BlockParser.parse(block_tokens)
                
                # Let SlotParser handle the condition, and pass the body_block
                stmt_node = SlotParser.parse_with_body(locked_stmt, body_block)

                # Check if this IfNode is followed by an else block
                if isinstance(stmt_node, IfNode):
                    peek_pos = pos
                    while peek_pos < len(tokens) and tokens[peek_pos].type in (TokenType.NEWLINE, TokenType.DEDENT):
                        peek_pos += 1

                    next_line_tokens = []
                    scan_pos = peek_pos
                    while scan_pos < len(tokens) and tokens[scan_pos].type not in (TokenType.NEWLINE, TokenType.EOF):
                        next_line_tokens.append(tokens[scan_pos])
                        scan_pos += 1

                    if next_line_tokens and next_line_tokens[-1].type == TokenType.SYMBOL and next_line_tokens[-1].value == ":":
                        check_line = next_line_tokens[:-1]
                        try:
                            next_locked = IntentDiscoveryEngine.process_statement(check_line)
                            if next_locked.intent_id == "COND_ELSE":
                                pos = scan_pos
                                if pos < len(tokens) and tokens[pos].type == TokenType.NEWLINE:
                                    pos += 1
                                if pos < len(tokens) and tokens[pos].type == TokenType.INDENT:
                                    pos += 1
                                    else_block_tokens = []
                                    else_indent = 1
                                    while pos < len(tokens):
                                        t = tokens[pos]
                                        if t.type == TokenType.INDENT:
                                            else_indent += 1
                                        elif t.type == TokenType.DEDENT:
                                            else_indent -= 1
                                            if else_indent == 0:
                                                pos += 1
                                                break
                                        else_block_tokens.append(t)
                                        pos += 1

                                    else_body_block = BlockParser.parse(else_block_tokens)
                                    stmt_node.else_body = else_body_block
                        except Exception:
                            pass

                statements.append(stmt_node)
            else:
                stmt_node = SlotParser.parse(locked_stmt)
                statements.append(stmt_node)
                
        return BlockNode(statements=statements)
