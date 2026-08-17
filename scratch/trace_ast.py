import sys
sys.path.insert(0, ".")
from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

with open("youtube.enlgs", "r", encoding="utf-8") as f:
    code = f.read()

tokens = ENLGSLexer(code).tokenize()
print(f"Total tokens: {len(tokens)}")

parser = ENLGSParser(tokens)
ast = parser.parse()
print(f"Parsed root body statements count: {len(ast.body)}")
for i, stmt in enumerate(ast.body):
    print(f"  {i}: {type(stmt).__name__} -> {getattr(stmt, 'name', '')} {getattr(stmt, 'target', '')}")

js = ENLGSEmitter(ast).emit()
print(f"Emitted JS length: {len(js)}")
with open("scratch/test_emit.js", "w", encoding="utf-8") as f:
    f.write(js)
