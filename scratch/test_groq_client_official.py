import sys
import os
sys.path.insert(0, os.path.abspath("."))
from groq import Groq
from enlg.ai.knowledge_base import get_enlang_system_prompt

key = "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"
client = Groq(api_key=key)

for m in ["openai/gpt-oss-120b", "groq/compound", "qwen/qwen3.8-27b", "openai/gpt-oss-20b"]:
    try:
        chat = client.chat.completions.create(
            model=m,
            messages=[
                {"role": "system", "content": get_enlang_system_prompt()},
                {"role": "user", "content": "How do I update team score in enlgs using natural syntax?"}
            ],
            temperature=0.2,
            max_tokens=500
        )
        print(f"[SUCCESS {m}]:\n")
        print(chat.choices[0].message.content)
        break
    except Exception as e:
        print(f"[FAILED {m}]:", e)
