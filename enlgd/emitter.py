"""enlgd CSS Emitter.

Traverses an ENLGD AST (StylesheetNode) and compiles it into valid, standard CSS.
"""

from typing import List
from .ast_nodes import (
    StylesheetNode, RuleNode, DeclarationNode,
    MediaRuleNode, VariableNode, KeyframeNode, KeyframeFrameNode
)

class ENLGDEmitter:
    """Emits standard CSS from an ENLGD StylesheetNode AST."""

    def __init__(self, stylesheet: StylesheetNode):
        self.stylesheet = stylesheet

    def emit(self) -> str:
        css_blocks: List[str] = []

        # 1. CSS Custom Properties (:root variables)
        if self.stylesheet.variables:
            var_lines = [":root {"]
            for var in self.stylesheet.variables:
                var_lines.append(f"  --{var.name}: {var.value};")
            var_lines.append("}")
            css_blocks.append("\n".join(var_lines))

        # 2. Keyframe Animations
        for kf in self.stylesheet.keyframes:
            kf_lines = [f"@keyframes {kf.name} {{"]
            for frame in kf.frames:
                kf_lines.append(f"  {frame.stop} {{")
                for decl in frame.declarations:
                    kf_lines.append(f"    {decl.property_name}: {decl.value};")
                kf_lines.append("  }")
            kf_lines.append("}")
            css_blocks.append("\n".join(kf_lines))

        # 3. Standard Rule Blocks
        for rule in self.stylesheet.rules:
            rule_str = self._emit_rule(rule)
            if rule_str:
                css_blocks.append(rule_str)

        # 4. Media Queries
        for media in self.stylesheet.media_rules:
            media_lines = [f"@media {media.query} {{"]
            for rule in media.rules:
                inner_rule = self._emit_rule(rule, indent=2)
                if inner_rule:
                    media_lines.append(inner_rule)
            media_lines.append("}")
            css_blocks.append("\n".join(media_lines))

        return "\n\n".join(css_blocks)

    def _emit_rule(self, rule: RuleNode, indent: int = 0) -> str:
        if not rule.declarations:
            return ""
        pad = " " * indent
        lines = [f"{pad}{rule.selector} {{"]
        for decl in rule.declarations:
            lines.append(f"{pad}  {decl.property_name}: {decl.value};")
        lines.append(f"{pad}}}")
        return "\n".join(lines)
