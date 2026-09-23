#!/usr/bin/env python3
"""
tools/enlang_lsp.py - The Sovereign Enlang Language Server Protocol Daemon (enlangg lsp)

Complies with Language Server Protocol (LSP 3.17) / JSON-RPC 2.0 over stdio:
  - Real-time syntax checking & red squiggly diagnostics (textDocument/publishDiagnostics)
  - Rich Markdown Hover Documentation for keywords, spatial primitives & builtins (textDocument/hover)
  - Context-Aware IntelliSense Completions & Snippets (textDocument/completion)
  - Automatic Document Formatting via Sovereign Formatter (textDocument/formatting)
"""

import sys
import os
import json
import re

# Ensure tools directory is in sys.path for enlang_fmt
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from enlang_fmt import format_source
except ImportError:
    format_source = lambda s: s

SERVER_CAPABILITIES = {
    "capabilities": {
        "textDocumentSync": {
            "openClose": True,
            "change": 1,  # Full sync
            "save": True
        },
        "hoverProvider": True,
        "completionProvider": {
            "resolveProvider": False,
            "triggerCharacters": [".", " ", "\"", "@"]
        },
        "documentFormattingProvider": True
    },
    "serverInfo": {
        "name": "enlangg-lsp",
        "version": "5.0.0-sovereign"
    }
}

HOVER_DOCUMENTATION = {
    "remember": "### `remember <var> as <value>`\nDeclares a mutable variable.\n\n```enlng\nremember counter as 0\ncounter increases by 1\n```",
    "freeze": "### `freeze <CONST> as <value>`\nDeclares an immutable constant.\n\n```enlng\nfreeze PI as 3.14159\n```",
    "when": "### `when <condition>:`\nNatural English conditional block header (terminates with `:`).\n\n```enlng\nwhen score >= 90:\n    show \"Grade: A\"\n```",
    "otherwise": "### `otherwise:` / `otherwise when <condition>:`\nAlternative branch for conditional statements.\n\n```enlng\nwhen score >= 90:\n    show \"A\"\notherwise when score >= 75:\n    show \"B\"\notherwise:\n    show \"C\"\n```",
    "for": "### `for each pair in <list>:` / `for <item> in <collection>:` / `for <i> from <start> to <end>:`\nUniversal iteration construct.\n\n```enlng\nfor each pair in numbers:\n    when pair.left > pair.right:\n        swap pair\n```",
    "repeat": "### `repeat while <cond>:` / `repeat until <cond>:` / `repeat <N> times:`\nLoop construct for bounded or conditional iteration.\n\n```enlng\nrepeat until sorted:\n    # pass\n```",
    "pair": "### `pair` (Spatial Algorithmic Primitive)\nIndex-free spatial primitive for pair traversal and sorting.\n\n- `pair.left` : Current left element\n- `pair.right` : Current right element\n- `swap pair` : Swap elements in-place",
    "pair.left": "### `pair.left`\nReturns the left element of the active spatial window pair.",
    "pair.right": "### `pair.right`\nReturns the right element of the active spatial window pair.",
    "swap": "### `swap <a> with <b>` / `swap pair`\nExchanges variables or spatial pair elements in-place with zero temporary variables.\n\n```enlng\nswap pair\nswap first with last\n```",
    "show": "### `show <expr...>` / `display <expr...>`\nOutputs evaluated expressions to standard output with natural space separation.\n\n```enlng\nshow \"Hello\", name\n```",
    "display": "### `display <expr...>`\nAlias for `show`.",
    "give": "### `give <expr>`\nReturns a value from a function or routine.\n\n```enlng\nfunction add with a, b:\n    give a + b\n```",
    "function": "### `function <name> with <args...>:`\nDefines a callable function.\n\n```enlng\nfunction fibonacci with n:\n    when n <= 1:\n        give n\n    give fibonacci(n - 1) + fibonacci(n - 2)\n```",
    "blueprint": "### `blueprint <Name>:`\nDeclares an OOP blueprint specification.\n\n```enlng\nblueprint Animal:\n    method speak:\n        show \"Generic animal sound\"\n```",
    "class": "### `class <Name> inherits from <Parent>:`\nDeclares an OOP class.\n\n```enlng\nclass Dog inherits from Animal:\n    method bark with volume:\n        show \"Woof at volume \" + str(volume)\n```",
    "method": "### `method <name> with <args...>:`\nDeclares a method inside a blueprint or class.\n\n```enlng\nmethod calculate_tax with rate:\n    give self.price * rate\n```",
    "tell": "### `tell <object> to <method> with <args...>`\nConversational dot-free method invocation.\n\n```enlng\ntell my_dog to bark with 10\n```",
    "ask": "### `ask <object> to <method>` / `ask <prompt>`\nConversational method invocation or user input prompt.\n\n```enlng\nask my_dog to speak\nname = ask \"Enter your name: \"\n```",
    "call": "### `call <method> on <object> with <args...>`\nConversational method invocation.\n\n```enlng\ncall bark on my_dog with 5\n```",
    "string_replace": "### `string_replace <val> in <s> at <pos>`\nNative spatial string replacement.\n\n```enlng\nstring_replace \"n\" in s at 0\nstring_replace \"n\" in s at_first\nstring_replace \"z\" in s at_last\n```",
    "string_add": "### `string_add <val> in <s> at <pos>`\nNative spatial string insertion / prepend / append.\n\n```enlng\nstring_add \"!\" in s at_last\n```",
    "string_remove": "### `string_remove from <s> at <pos>`\nNative spatial string deletion.\n\n```enlng\nstring_remove from s at 0\nstring_remove from s at_first\nstring_remove from s at_last\n```",
    "at_first": "### `at_first`\nSpatial position specifier representing index 0.",
    "at_last": "### `at_last`\nSpatial position specifier representing the last index (-1).",
    "use": "### `use <module_name>`\nImports an Enlang module, standard library, or Python library.\n\n```enlng\nuse \"lib_math.enlng\"\nuse \"math\"\n```",
    "add": "### `add <val> to <list>`\nAppends an element to a dynamic list.\n\n```enlng\nadd 50 to items\n```",
    "remove": "### `remove <val> from <list>`\nRemoves first occurrence of value from list.\n\n```enlng\nremove 20 from items\n```",
    "count": "### `count of <collection>`\nReturns the number of elements in a list, map, or string.\n\n```enlng\ntotal = count of items\n```"
}

COMPLETIONS = [
    # Keywords
    {"label": "remember", "kind": 14, "detail": "remember <var> as <val>", "insertText": "remember ${1:var_name} as ${2:value}"},
    {"label": "freeze", "kind": 14, "detail": "freeze <CONST> as <val>", "insertText": "freeze ${1:CONST_NAME} as ${2:value}"},
    {"label": "when", "kind": 14, "detail": "when <condition>:", "insertText": "when ${1:condition}:\n    ${0}"},
    {"label": "otherwise", "kind": 14, "detail": "otherwise:", "insertText": "otherwise:\n    ${0}"},
    {"label": "otherwise when", "kind": 14, "detail": "otherwise when <cond>:", "insertText": "otherwise when ${1:condition}:\n    ${0}"},
    {"label": "for each pair", "kind": 15, "detail": "Index-free pair traversal", "insertText": "for each pair in ${1:numbers}:\n    when pair.left > pair.right:\n        swap pair"},
    {"label": "for item in", "kind": 15, "detail": "Container traversal", "insertText": "for ${1:item} in ${2:collection}:\n    show ${1:item}"},
    {"label": "for range", "kind": 15, "detail": "Bounded range loop", "insertText": "for ${1:i} from ${2:0} to ${3:10} by ${4:1}:\n    ${0}"},
    {"label": "repeat while", "kind": 15, "detail": "While condition loop", "insertText": "repeat while ${1:condition}:\n    ${0}"},
    {"label": "repeat until", "kind": 15, "detail": "Until condition loop", "insertText": "repeat until ${1:condition}:\n    ${0}"},
    {"label": "repeat times", "kind": 15, "detail": "Fixed repeat loop", "insertText": "repeat ${1:10} times:\n    ${0}"},
    {"label": "function", "kind": 15, "detail": "Define function", "insertText": "function ${1:name} with ${2:args}:\n    give ${0}"},
    {"label": "blueprint", "kind": 15, "detail": "Define OOP Blueprint", "insertText": "blueprint ${1:Name}:\n    method ${2:speak}:\n        ${0}"},
    {"label": "class", "kind": 15, "detail": "Define Class", "insertText": "class ${1:Dog} inherits from ${2:Animal}:\n    method __init__ with ${3:name}:\n        self.name = ${3:name}\n    ${0}"},
    {"label": "swap", "kind": 14, "detail": "swap <a> with <b>", "insertText": "swap ${1:a} with ${2:b}"},
    {"label": "swap pair", "kind": 14, "detail": "swap active spatial pair", "insertText": "swap pair"},
    {"label": "show", "kind": 14, "detail": "show <expr>", "insertText": "show ${0}"},
    {"label": "give", "kind": 14, "detail": "give <expr>", "insertText": "give ${0}"},
    {"label": "tell", "kind": 14, "detail": "tell <obj> to <method> with <args>", "insertText": "tell ${1:obj} to ${2:method} with ${3:args}"},
    {"label": "ask", "kind": 14, "detail": "ask <prompt>", "insertText": "ask \"${1:prompt}\""},
    {"label": "use", "kind": 14, "detail": "use \"<module>\"", "insertText": "use \"${1:lib_math.enlng}\""},
    {"label": "pair.left", "kind": 10, "detail": "Spatial pair left element", "insertText": "pair.left"},
    {"label": "pair.right", "kind": 10, "detail": "Spatial pair right element", "insertText": "pair.right"},
    {"label": "at_first", "kind": 14, "detail": "Index 0 specifier", "insertText": "at_first"},
    {"label": "at_last", "kind": 14, "detail": "Last index specifier", "insertText": "at_last"},
    {"label": "string_replace", "kind": 3, "detail": "string_replace <val> in <s> at <pos>", "insertText": "string_replace \"${1:new}\" in ${2:s} at ${3:at_first}"},
    {"label": "string_add", "kind": 3, "detail": "string_add <val> in <s> at <pos>", "insertText": "string_add \"${1:!}\" in ${2:s} at ${3:at_last}"},
    {"label": "string_remove", "kind": 3, "detail": "string_remove from <s> at <pos>", "insertText": "string_remove from ${1:s} at ${2:at_first}"},
    # Standard Libraries
    {"label": "\"lib_math.enlng\"", "kind": 9, "detail": "Math Standard Library", "insertText": "\"lib_math.enlng\""},
    {"label": "\"lib_strings.enlng\"", "kind": 9, "detail": "Strings Standard Library", "insertText": "\"lib_strings.enlng\""},
    {"label": "\"lib_ds.enlng\"", "kind": 9, "detail": "Data Structures (Stack, Queue, Graph)", "insertText": "\"lib_ds.enlng\""},
    {"label": "\"lib_file.enlng\"", "kind": 9, "detail": "File I/O Library", "insertText": "\"lib_file.enlng\""},
    {"label": "\"lib_fs.enlng\"", "kind": 9, "detail": "File System Operations", "insertText": "\"lib_fs.enlng\""},
    {"label": "\"lib_ml.enlng\"", "kind": 9, "detail": "Machine Learning & Vector Ops", "insertText": "\"lib_ml.enlng\""},
    {"label": "\"lib_net.enlng\"", "kind": 9, "detail": "HTTP & Socket Networking", "insertText": "\"lib_net.enlng\""},
    {"label": "\"lib_db.enlng\"", "kind": 9, "detail": "Enlang Database Engine", "insertText": "\"lib_db.enlng\""},
    {"label": "\"lib_concurrency.enlng\"", "kind": 9, "detail": "Multi-Threaded Concurrency", "insertText": "\"lib_concurrency.enlng\""},
    {"label": "\"lib_std.enlng\"", "kind": 9, "detail": "Core Standard Helpers", "insertText": "\"lib_std.enlng\""}
]

BLOCK_HEADER_KEYWORDS = (
    "when", "if", "while", "repeat while", "repeat until", "until", 
    "otherwise when", "else if", "elif", "otherwise", "else", 
    "function", "define function", "def", "procedure", "routine",
    "for", "blueprint", "class", "method"
)

TYPO_KEYWORDS = [
    "remember", "freeze", "when", "otherwise", "function", "repeat", 
    "for", "swap", "pair", "show", "display", "give", "blueprint", 
    "class", "method", "tell", "ask", "call", "use", "add", "remove"
]

def check_diagnostics(source_text: str):
    """
    Validates Enlang source code in real time and produces LSP diagnostic list.
    """
    diagnostics = []
    lines = source_text.splitlines()

    for line_idx, line in enumerate(lines):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        # Strip inline comment
        c_idx = stripped.find("#")
        code_part = stripped[:c_idx].rstrip() if c_idx != -1 else stripped

        # 1. Missing Colon on Block Headers
        for bkw in BLOCK_HEADER_KEYWORDS:
            is_match = False
            if code_part == bkw:
                is_match = True
            elif code_part.lower().startswith(bkw + " "):
                is_match = True

            if is_match:
                if not code_part.endswith(":"):
                    diagnostics.append({
                        "range": {
                            "start": {"line": line_idx, "character": len(line) - len(line.lstrip())},
                            "end": {"line": line_idx, "character": len(line)}
                        },
                        "severity": 1,  # Error
                        "source": "enlangg",
                        "message": f"SyntaxError: Missing colon ':' at end of '{bkw}' block header."
                    })
                break

        # 2. Unclosed string literals
        quote_chars = [c for c in code_part if c in ('"', "'")]
        # Count non-escaped quotes
        double_quotes = 0
        single_quotes = 0
        esc = False
        for ch in code_part:
            if esc:
                esc = False
                continue
            if ch == '\\':
                esc = True
                continue
            if ch == '"': double_quotes += 1
            elif ch == "'": single_quotes += 1

        if (double_quotes % 2 != 0) or (single_quotes % 2 != 0):
            diagnostics.append({
                "range": {
                    "start": {"line": line_idx, "character": 0},
                    "end": {"line": line_idx, "character": len(line)}
                },
                "severity": 1,  # Error
                "source": "enlangg",
                "message": "SyntaxError: EOL while scanning string literal (unclosed quotation mark)."
            })

        # 3. Unmatched brackets or parentheses on single line
        p_depth = code_part.count('(') - code_part.count(')')
        b_depth = code_part.count('[') - code_part.count(']')
        c_depth = code_part.count('{') - code_part.count('}')
        if p_depth > 0:
            diagnostics.append({
                "range": {
                    "start": {"line": line_idx, "character": 0},
                    "end": {"line": line_idx, "character": len(line)}
                },
                "severity": 1,
                "source": "enlangg",
                "message": f"SyntaxError: Unclosed parenthesis '(' (missing {p_depth} closing paren(s))."
            })
        elif p_depth < 0:
            diagnostics.append({
                "range": {
                    "start": {"line": line_idx, "character": 0},
                    "end": {"line": line_idx, "character": len(line)}
                },
                "severity": 1,
                "source": "enlangg",
                "message": "SyntaxError: Unmatched closing parenthesis ')'."
            })

    return diagnostics

class EnlangLSP:
    """Standard JSON-RPC 2.0 LSP 3.17 stdio server for Enlang."""

    def __init__(self):
        self.documents = {}
        self.running = True

    def read_message(self):
        """Reads a single JSON-RPC message from sys.stdin.buffer."""
        content_length = None
        while True:
            line = sys.stdin.buffer.readline()
            if not line:
                return None
            line_str = line.decode("latin1").strip()
            if not line_str:
                break
            if line_str.lower().startswith("content-length:"):
                content_length = int(line_str.split(":")[1].strip())

        if content_length is None:
            return None

        body = sys.stdin.buffer.read(content_length)
        if not body:
            return None
        return json.loads(body.decode("utf-8"))

    def send_message(self, msg_dict):
        """Sends a JSON-RPC message to sys.stdout.buffer with Content-Length."""
        payload = json.dumps(msg_dict, separators=(',', ':')).encode("utf-8")
        header = f"Content-Length: {len(payload)}\r\n\r\n".encode("latin1")
        sys.stdout.buffer.write(header)
        sys.stdout.buffer.write(payload)
        sys.stdout.buffer.flush()

    def send_response(self, req_id, result):
        self.send_message({
            "jsonrpc": "2.0",
            "id": req_id,
            "result": result
        })

    def send_notification(self, method, params):
        self.send_message({
            "jsonrpc": "2.0",
            "method": method,
            "params": params
        })

    def publish_diagnostics(self, uri):
        text = self.documents.get(uri, "")
        diags = check_diagnostics(text)
        self.send_notification("textDocument/publishDiagnostics", {
            "uri": uri,
            "diagnostics": diags
        })

    def handle_hover(self, uri, line, col):
        text = self.documents.get(uri, "")
        lines = text.splitlines()
        if line >= len(lines):
            return None
        target_line = lines[line]
        if col > len(target_line):
            return None

        # Check for pair.left / pair.right
        m_pair = re.search(r'\bpair\.(left|right)\b', target_line)
        if m_pair and m_pair.start() <= col <= m_pair.end():
            token = m_pair.group(0)
            if token in HOVER_DOCUMENTATION:
                return {"contents": {"kind": "markdown", "value": HOVER_DOCUMENTATION[token]}}

        # Extract word token at col
        start = col
        while start > 0 and (target_line[start - 1].isalnum() or target_line[start - 1] == '_'):
            start -= 1
        end = col
        while end < len(target_line) and (target_line[end].isalnum() or target_line[end] == '_'):
            end += 1

        word = target_line[start:end]
        if word in HOVER_DOCUMENTATION:
            return {"contents": {"kind": "markdown", "value": HOVER_DOCUMENTATION[word]}}

        return None

    def handle_completion(self, uri, line, col):
        return {
            "isIncomplete": False,
            "items": COMPLETIONS
        }

    def handle_formatting(self, uri):
        text = self.documents.get(uri, "")
        if not text:
            return []
        formatted = format_source(text)
        line_count = len(text.splitlines()) + 5
        return [
            {
                "range": {
                    "start": {"line": 0, "character": 0},
                    "end": {"line": line_count, "character": 0}
                },
                "newText": formatted
            }
        ]

    def run(self):
        """Main JSON-RPC event loop."""
        while self.running:
            try:
                msg = self.read_message()
            except Exception as e:
                sys.stderr.write(f"[ENLANGG LSP READ ERROR] {e}\n")
                sys.stderr.flush()
                break

            if msg is None:
                break

            method = msg.get("method")
            req_id = msg.get("id")
            params = msg.get("params", {})

            # 1. Lifecycle
            if method == "initialize":
                self.send_response(req_id, SERVER_CAPABILITIES)
            elif method == "initialized":
                pass
            elif method == "shutdown":
                self.send_response(req_id, None)
            elif method == "exit":
                self.running = False
                break

            # 2. Document Sync
            elif method == "textDocument/didOpen":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                text = td.get("text", "")
                self.documents[uri] = text
                self.publish_diagnostics(uri)

            elif method == "textDocument/didChange":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                changes = params.get("contentChanges", [])
                if changes:
                    self.documents[uri] = changes[-1].get("text", "")
                    self.publish_diagnostics(uri)

            elif method == "textDocument/didSave":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                self.publish_diagnostics(uri)

            elif method == "textDocument/didClose":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                self.documents.pop(uri, None)

            # 3. Language Features
            elif method == "textDocument/hover":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                pos = params.get("position", {})
                res = self.handle_hover(uri, pos.get("line", 0), pos.get("character", 0))
                self.send_response(req_id, res)

            elif method == "textDocument/completion":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                pos = params.get("position", {})
                res = self.handle_completion(uri, pos.get("line", 0), pos.get("character", 0))
                self.send_response(req_id, res)

            elif method == "textDocument/formatting":
                td = params.get("textDocument", {})
                uri = td.get("uri")
                res = self.handle_formatting(uri)
                self.send_response(req_id, res)

            elif req_id is not None:
                # Default response for unsupported requests
                self.send_response(req_id, None)

def main():
    lsp = EnlangLSP()
    lsp.run()

if __name__ == "__main__":
    main()
