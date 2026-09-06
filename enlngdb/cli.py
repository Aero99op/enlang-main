"""CLI entrypoint for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

import sys
import argparse
from enlngdb.compiler import run_enlngdb_file, run_enlngdb_source


def main():
    parser = argparse.ArgumentParser(
        prog="enlngdb",
        description="EnLang Native Sovereign Database Engine (ZERO SQL, In-Memory & Disk Persistence)"
    )
    parser.add_argument("file", nargs="?", help="Path to .enlngdb script file to execute")
    parser.add_argument("--eval", "-e", help="Inline enlngdb code string to execute")
    parser.add_argument("--db", help="Path to sovereign database persistence file (.edb)")
    parser.add_argument("--quiet", "-q", action="store_true", help="Suppress streaming output")

    args = parser.parse_args()

    if args.eval:
        reports = run_enlngdb_source(args.eval, db_path=args.db, stream_output=not args.quiet)
        failed = any(not r.get("success", False) for r in reports)
        sys.exit(1 if failed else 0)

    if args.file:
        reports = run_enlngdb_file(args.file, db_path=args.db, stream_output=not args.quiet)
        failed = any(not r.get("success", False) for r in reports)
        sys.exit(1 if failed else 0)

    parser.print_help()
    sys.exit(0)


if __name__ == "__main__":
    main()
