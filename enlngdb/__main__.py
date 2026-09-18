"""enlngdb CLI Entry Point.
Supports:
  python -m enlngdb <script.enlngdb>
"""
import sys
from enlngdb.compiler import run_enlngdb_file

def main():
    if len(sys.argv) < 2:
        print("Usage: python -m enlngdb <script.enlngdb>")
        sys.exit(1)
    run_enlngdb_file(sys.argv[1])

if __name__ == "__main__":
    main()
