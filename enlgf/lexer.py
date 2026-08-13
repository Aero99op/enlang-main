"""enlgf Lexical Analyzer.

Scans raw .enlgf English markup source code and emits structured Tokens.
Handles indentation tracking, quoted string extraction, and multi-word phrase matching.
"""

from typing import List
from .tokens import Token, TokenType, TAG_MAPPINGS, EVENT_MAPPINGS, ATTR_MAPPINGS

def _strip_comment(line: str) -> str:
    in_quote = False
    quote_char = None
    i = 0
    while i < len(line):
        c = line[i]
        if in_quote:
            if c == quote_char:
                in_quote = False
            elif c == '\\':
                i += 1
        else:
            if c in ('"', "'"):
                in_quote = True
                quote_char = c
            elif c == '#':
                return line[:i]
        i += 1
    return line

class ENLGFLexer:
    """Tokenizes .enlgf source code."""
    
    def __init__(self, source: str):
        self.source = source
        self.tokens: List[Token] = []
        self.line_num = 1
        self.indent_stack = [0]
        
    def tokenize(self) -> List[Token]:
        lines = self.source.splitlines()
        
        for self.line_num, line in enumerate(lines, 1):
            line_no_comment = _strip_comment(line)
            if not line_no_comment.strip():
                continue
                
            # Compute indentation
            indent_length = len(line_no_comment) - len(line_no_comment.lstrip())
            
            if indent_length > self.indent_stack[-1]:
                self.indent_stack.append(indent_length)
                self.tokens.append(Token(TokenType.INDENT, "", self.line_num, 1))
            elif indent_length < self.indent_stack[-1]:
                while self.indent_stack and indent_length < self.indent_stack[-1]:
                    self.indent_stack.pop()
                    self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))
                    
            raw_line = line_no_comment.lstrip()
            self._tokenize_line(raw_line)
            self.tokens.append(Token(TokenType.NEWLINE, "\n", self.line_num, len(line)))
            
        # Unwind indents at EOF
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))
            
        self.tokens.append(Token(TokenType.EOF, "", self.line_num, 1))
        return self.tokens

    def _tokenize_line(self, line: str):
        i = 0
        length = len(line)
        
        while i < length:
            # Skip spaces
            if line[i] in (' ', '\t'):
                i += 1
                continue
                
            # Handle Symbols like ':' or ',' or '='
            if line[i] in (':', ',', '='):
                self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1))
                i += 1
                continue
                
            # Handle Strings "..." or '...'
            if line[i] in ('"', "'"):
                quote_char = line[i]
                start = i
                i += 1
                val = ""
                while i < length and line[i] != quote_char:
                    if line[i] == '\\' and i + 1 < length:
                        i += 1
                        val += line[i]
                    else:
                        val += line[i]
                    i += 1
                if i < length and line[i] == quote_char:
                    i += 1
                self.tokens.append(Token(TokenType.STRING, val, self.line_num, start + 1))
                continue
                
            # Check multi-word or single-word phrases against TAG_MAPPINGS, EVENT_MAPPINGS
            remainder = line[i:].lower()
            matched_phrase = None
            
            # Check longest matching phrase in TAG_MAPPINGS or EVENT_MAPPINGS first
            all_phrases = sorted(
                list(TAG_MAPPINGS.keys()) + list(EVENT_MAPPINGS.keys()),
                key=len,
                reverse=True
            )
            
            for phrase in all_phrases:
                if remainder.startswith(phrase):
                    # Ensure word boundary check
                    phrase_len = len(phrase)
                    if phrase_len == len(remainder) or not remainder[phrase_len].isalnum():
                        matched_phrase = phrase
                        token_type = TokenType.KEYWORD
                        self.tokens.append(Token(token_type, matched_phrase, self.line_num, i + 1))
                        i += phrase_len
                        break
                        
            if matched_phrase:
                continue
                
            # Handle Numbers
            if line[i].isdigit():
                start = i
                while i < length and (line[i].isdigit() or line[i] == '.' or line[i:i+2].lower() in ('px', 'em', 'rem', '%', 'vh', 'vw')):
                    if line[i:i+2].lower() in ('px', 'em', 'rem', 'vh', 'vw'):
                        i += 2
                        break
                    elif line[i] == '%':
                        i += 1
                        break
                    i += 1
                self.tokens.append(Token(TokenType.NUMBER, line[start:i], self.line_num, start + 1))
                continue
                
            # Handle Identifiers/Words
            if line[i].isalpha() or line[i] in ('_', '-'):
                start = i
                while i < length and (line[i].isalnum() or line[i] in ('_', '-')):
                    i += 1
                val = line[start:i]
                self.tokens.append(Token(TokenType.IDENTIFIER, val, self.line_num, start + 1))
                continue
                
            # Fallback single char
            self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1))
            i += 1
