import sys
from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

source = "const renderer = new THREE.WebGLRenderer({ canvas: canvas, antialias: true, alpha: true });"
print("SOURCE:", source)
tokens = ENLGSLexer(source).tokenize()
print("TOKENS:", [t.value for t in tokens])
ast = ENLGSParser(tokens).parse()
js = ENLGSEmitter(ast).emit()
print("JS:", js)
