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
                
            # Check if this line ends with a colon indicating a block follows
            expects_block = False
            if line_tokens[-1].type == TokenType.SYMBOL and line_tokens[-1].value == ":":
                expects_block = True
                line_tokens = line_tokens[:-1] # Remove colon
                
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
                statements.append(stmt_node)
            else:
                stmt_node = SlotParser.parse(locked_stmt)
                statements.append(stmt_node)
                
        return BlockNode(statements=statements)
