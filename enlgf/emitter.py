"""enlgf HTML & JS Emitter.

Traverses an ENLGF AST and compiles it into 100% valid HTML5 with inline styles and JS behaviors.
"""

from typing import List
from .ast_nodes import DocumentNode, ElementNode, TextNode, RawHTMLNode, ASTNode

class ENLGFEmitter:
    """Emits HTML5 string from ENLGF AST."""
    
    def __init__(self, doc: DocumentNode):
        self.doc = doc
        self.indent_level = 0
        
    def emit(self) -> str:
        html = ["<!DOCTYPE html>"]
        
        # Document Root Attributes
        lang_attr = f' lang="{self.doc.attributes["lang"]}"' if "lang" in self.doc.attributes else ' lang="en"'
        html.append(f'<html{lang_attr}>')
        
        # Head Section
        html.append('  <head>')
        html.append('    <meta charset="UTF-8">')
        html.append('    <meta name="viewport" content="width=device-width, initial-scale=1.0">')
        for child in self.doc.head_children:
            html.append(self._emit_node(child, depth=2))
        html.append('  </head>')
        
        # Body Section
        html.append('  <body>')
        
        body_nodes = self.doc.body_children if self.doc.body_children else self.doc.children
        for child in body_nodes:
            html.append(self._emit_node(child, depth=2))
            
        html.append('  </body>')
        html.append('</html>')
        
        return "\n".join(html)

    def _emit_node(self, node: ASTNode, depth: int = 0) -> str:
        indent = "  " * depth
        
        if isinstance(node, TextNode):
            return f"{indent}{node.text}"

        if isinstance(node, RawHTMLNode):
            return f"{indent}{node.content}"
            
        if isinstance(node, ElementNode):
            # Assemble attributes
            attrs_str = ""
            for k, v in node.attributes.items():
                attrs_str += f' {k}="{v}"'
                
            # Assemble inline styles
            if node.styles:
                style_content = "; ".join([f"{k}: {v}" for k, v in node.styles.items()])
                attrs_str += f' style="{style_content}"'
                
            # Assemble JS inline events
            for event_name, event_code in node.events.items():
                attrs_str += f' {event_name}="{event_code}"'
                
            # Self closing tag
            if node.is_self_closing:
                return f"{indent}<{node.tag}{attrs_str}>"
                
            # Inline elements or text-only elements
            if node.text_content and not node.children:
                return f"{indent}<{node.tag}{attrs_str}>{node.text_content}</{node.tag}>"
                
            # Nested elements
            lines = [f"{indent}<{node.tag}{attrs_str}>"]
            if node.text_content:
                lines.append(f"{indent}  {node.text_content}")
                
            for child in node.children:
                lines.append(self._emit_node(child, depth + 1))
                
            lines.append(f"{indent}</{node.tag}>")
            return "\n".join(lines)
            
        return ""
