"""Comprehensive Test for 'type <extension>' header across all 6 Enlang Domains."""

import sys
import os

repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, repo_root)

print("=" * 70)
print("  TESTING 'type <extension>' HEADER ACROSS ALL 6 DOMAINS")
print("=" * 70)

# 1. Domain: .enlg (Core Backend Logic)
from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

enlg_code = """type enlg

declare count = 10
declare sum = 0
while count > 0:
    set sum = sum + count
    set count = count - 1

print "Sum from 10 down to 1 is: " + sum
"""
tokens = Lexer(enlg_code).tokenize()
ast = BlockParser.parse(tokens)
cir = CIRGenerator().generate(ast)
vm = VirtualMachine()
vm.execute(cir)
assert vm.environment.get("sum") == 55
print("  [PASS] Domain 1: 'type enlg' -> Executed perfectly in VM (sum = 55)")

# 2. Domain: .enlgdb (Natural SQL Database)
from enlgdb.lexer import Lexer as DBLexer
from enlgdb.parser import Parser as DBParser
from enlgdb.emitter import SQLEmitter
from enlgdb.engine import DatabaseEngine

enlgdb_code = """type enlgdb

create table "guilds" with:
    id as integer primary key autoincrement
    guild_name as text not null unique
    member_count as integer default 1

insert into "guilds" values:
    guild_name: "ShadowAssassins"
    member_count: 42

select all from "guilds" where member_count >= 10
"""
db_tokens = DBLexer(enlgdb_code).tokenize()
db_ast = DBParser(db_tokens).parse()
db_emitter = SQLEmitter(dialect="sqlite")
db_engine = DatabaseEngine(":memory:")
reports = db_engine.execute_program(db_ast, db_emitter)
assert reports[-1]["rows"][0]["guild_name"] == "ShadowAssassins"
print("  [PASS] Domain 2: 'type enlgdb' -> Table created, inserted, queried on SQLite")

# 3. Domain: .enlgf (Frontend HTML DSL)
from enlgf.lexer import ENLGFLexer
from enlgf.parser import ENLEGFPParser
from enlgf.emitter import ENLGFEmitter

enlgf_code = """type enlgf

document enlgf:
    head:
        title "Esports Arena"
    body:
        header class "hero-box":
            heading 1 "Championship 2026"
            paragraph "Live stream starting soon"
        button "Join Queue" id "btn-join"
"""
f_tokens = ENLGFLexer(enlgf_code).tokenize()
f_ast = ENLEGFPParser(f_tokens).parse()
f_html = ENLGFEmitter(f_ast).emit()
assert "<!DOCTYPE html>" in f_html
assert "<title>Esports Arena</title>" in f_html
assert '<button id="btn-join">Join Queue</button>' in f_html
print("  [PASS] Domain 3: 'type enlgf' -> Compiled to standard HTML5 document")

# 4. Domain: .enlgd (Design CSS DSL)
from enlgd.lexer import ENLGDLexer
from enlgd.parser import ENLGDParser
from enlgd.emitter import ENLGDEmitter

enlgd_code = """type enlgd

define color "accent" as "#8b5cf6"

for ".hero-box" apply:
    background: "#0f172a"
    color: "#f8fafc"
    padding: "32px"
end

when "#btn-join" is hovered apply:
    background: "#8b5cf6"
    transform: "scale(1.05)"
end
"""
d_tokens = ENLGDLexer(enlgd_code).tokenize()
d_ast = ENLGDParser(d_tokens).parse()
d_css = ENLGDEmitter(d_ast).emit()
assert "--accent: #8b5cf6;" in d_css
assert ".hero-box {" in d_css
assert "#btn-join:hover {" in d_css
print("  [PASS] Domain 4: 'type enlgd' -> Compiled to standard CSS stylesheet")

# 5. Domain: .enlgs (Reactive Scripting DSL)
from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

enlgs_code = """type enlgs

in script:
    create score as 100

    when "btn-join" is clicked:
        set score = score + 25
        refresh "hero-box" with "Score: " + score
"""
s_tokens = ENLGSLexer(enlgs_code).tokenize()
s_ast = ENLGSParser(s_tokens).parse()
s_js = ENLGSEmitter(s_ast).emit()
assert "let score = 100;" in s_js
assert "addEventListener" in s_js
print("  [PASS] Domain 5: 'type enlgs' -> Compiled to reactive browser JS")

# 6. Domain: .enlgm (Mobile Flutter DSL)
from enlgm.lexer import ENLGMLexer
from enlgm.parser import ENLGMParser
from enlgm.emitter import ENLGMEmitter

enlgm_code = """type enlgm

in mobile:
    screen HomeScreen:
        body:
            text "Hello Mobile"
"""
m_tokens = ENLGMLexer(enlgm_code).tokenize()
m_ast = ENLGMParser(m_tokens).parse()
m_dart = ENLGMEmitter(m_ast).emit()
assert "class HomeScreen extends StatelessWidget" in m_dart
assert "Hello Mobile" in m_dart
print("  [PASS] Domain 6: 'type enlgm' -> Compiled to Flutter Dart widget tree")

print("=" * 70)
print("  ALL 6 DOMAINS UNIVERSALLY SUPPORT 'type <extension>' HEADERS!")
print("=" * 70)
