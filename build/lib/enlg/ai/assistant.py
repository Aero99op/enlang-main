"""Enlang AI Assistant CLI Interface.

Supports one-shot questions and interactive terminal conversational REPL.
"""

import sys
import os

# Ensure clean UTF-8 printing in Windows console without charmap crashes
if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

from .groq_client import query_groq

def run_ai_cli(prompt: str = None):
    """Starts an interactive session or processes a single prompt."""
    if prompt and prompt.strip():
        print("Thinking...", end="", flush=True)
        response = query_groq(prompt.strip())
        print("\r" + " " * 15 + "\r", end="", flush=True)
        print("================================================================")
        print("  EnLang AI Assistant:")
        print("================================================================")
        print(response)
        print("================================================================")
        return

    # Interactive session
    print("================================================================")
    print("  EnLang AI Interactive Assistant (Powered by Groq)")
    print("================================================================")
    print("  Ask anything about Enlang syntax, UI, backend, or code generation.")
    print("  Type 'exit' or 'quit' to end the session.\n")

    history = []

    while True:
        try:
            user_input = input("enlang-ai> ").strip()
            if not user_input:
                continue
            if user_input.lower() in ("exit", "quit", "q"):
                print("Exiting Enlang AI. Goodbye!")
                break

            print("Thinking...", end="", flush=True)
            reply = query_groq(user_input, history=history)
            print("\r" + " " * 15 + "\r", end="", flush=True)

            print("\n" + reply + "\n")
            
            history.append({"role": "user", "content": user_input})
            history.append({"role": "assistant", "content": reply})
            
            # Keep history manageable
            if len(history) > 10:
                history = history[-10:]

        except KeyboardInterrupt:
            print("\nExiting Enlang AI.")
            break
        except Exception as e:
            print(f"\nError: {e}")
