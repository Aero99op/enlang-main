import json
import urllib.request
import urllib.error

key = "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"

candidate_models = [
    "deepseek-r1-distill-llama-70b",
    "llama-3.3-70b-specdec",
    "llama-3.2-11b-vision-preview",
    "llama-3.2-3b-preview",
    "llama-3.2-1b-preview",
    "qwen-2.5-32b",
    "gemma2-9b-it",
    "llama-3.3-70b-versatile",
    "llama-3.1-8b-instant"
]

for m in candidate_models:
    payload = {
        "model": m,
        "messages": [{"role": "user", "content": "Say hello from Enlang in 3 words!"}],
        "max_tokens": 30
    }
    req = urllib.request.Request(
        "https://api.groq.com/openai/v1/chat/completions",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {key}",
            "Content-Type": "application/json",
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        }
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read().decode())
            print(f"[ACTIVE]: {m} -> {data['choices'][0]['message']['content'].strip()}")
    except urllib.error.HTTPError as e:
        print(f"[HTTP {e.code}]: {m} -> {e.read().decode()[:80]}")
    except Exception as e:
        print(f"[ERROR]: {m} -> {e}")
