# enlang Ecosystem Project Baseline

## 1. Project Status & Clean Slate Rule
- **Old enlg Project:** RETIRED. The legacy registry, grammar, AST, CIR, backend, runtime, and EnLangDB assumptions have been completely abandoned.
- **New Project Foundation:** Starts from absolute zero. 
- **Legacy Files:** The previous workspace has been wiped clean.

## 2. Core Vision: The Five-Language Ecosystem
This project establishes a complete English-first programming ecosystem consisting of five deterministic, interoperable languages:
1. **enlg:** General-purpose programming language (Python equivalent).
2. **enlgf:** Document and structure language (HTML equivalent).
3. **enlgd:** Presentation and style language (CSS equivalent).
4. **enlgs:** Behavior and interaction language (JavaScript equivalent).
5. **enlgdb:** Database and data-manipulation language (SQL equivalent).

## 3. Absolute Design Principles
- **Natural English-Oriented Programming:** English-first syntax that feels understandable to read, combined with strictly deterministic compiler semantics.
- **Hint-Keyword / Intent Model:** A hint keyword identifies the semantic intent (WHAT to do). The remaining tokens define operands/modifiers flexibly.
- **Flexibility Without Ambiguity:** The compiler parses remaining tokens based on locked intent. If ambiguity exists, it **FAILS CLOSED**. It will *never* guess.
- **Python-Level Capability:** enlg is not an educational toy. It must support normal programming, AI/ML, Cloud, Web, DevOps, and Data Science, natively interoperating with the Python ecosystem.

## 4. Implementation Directives
- **enlg First:** enlg is the foundation. Implementation of enlgf, enlgd, enlgs, and enlgdb will NOT begin until the enlg core is architecturally stable.
- **Bounded Roadmap:** The ecosystem will be built in exactly 20 phases. Phase 20 is the absolute final release certification. No Phase 21+ exists.
- **Phase Contracts:** Each phase defines strict architectural scope, boundaries, and tests. Frozen phases cannot be casually modified without formal architectural review.
