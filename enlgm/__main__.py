"""enlgm CLI Entry Point.
Supports:
  enlngm <file.enlngm> [-o <out.dart>]
  enlngm build <file.enlngm> [--target <apk|ipa>] [-o <out>]
  enlngm --version
"""
import sys
import os

def main():
    args = sys.argv[1:]
    if not args or "--help" in args or "-h" in args:
        print("enlngm - Enlangg Mobile Native & HAL Compiler v5.0.0")
        print("\nUsage:")
        print("  enlngm <file.enlngm> [-o <out.dart>]       Compile mobile screen to Flutter/Dart")
        print("  enlngm build <file.enlngm> --target <apk|ipa> -o <out> Build mobile package")
        print("  enlngm --version                           Display version information")
        sys.exit(0)

    if "--version" in args or "-v" in args:
        print("enlngm version 5.0.0 (Enlangg Mobile Native & HAL Compiler)")
        sys.exit(0)

    if args[0] == "build":
        if len(args) < 2:
            print("Usage: enlngm build <file.enlngm> [--target <apk|ipa>] [-o <out>]", file=sys.stderr)
            sys.exit(1)
        src = args[1]
        target = "apk"
        out = "app.apk"
        for i in range(2, len(args)):
            if args[i] == "--target" and i + 1 < len(args):
                target = args[i + 1].lower()
            elif args[i] == "-o" and i + 1 < len(args):
                out = args[i + 1]

        from enlgm.compiler import compile_enlgm_file
        dart_code = compile_enlgm_file(src)

        proj_dir = os.path.splitext(src)[0] + "_flutter_project"
        lib_dir = os.path.join(proj_dir, "lib")
        os.makedirs(lib_dir, exist_ok=True)
        main_dart_path = os.path.join(lib_dir, "main.dart")
        with open(main_dart_path, "w", encoding="utf-8") as f:
            f.write(dart_code)

        pubspec_path = os.path.join(proj_dir, "pubspec.yaml")
        if not os.path.exists(pubspec_path):
            with open(pubspec_path, "w", encoding="utf-8") as f:
                f.write("""name: mobile_app
description: Generated Enlang Mobile Application
version: 1.0.0+1
environment:
  sdk: '>=3.0.0 <4.0.0'
dependencies:
  flutter:
    sdk: flutter
  http: ^1.2.0
flutter:
  uses-material-design: true
""")

        print("==============================================================")
        print("       ENLANG MOBILE PRODUCTION COMPILER                      ")
        print("==============================================================")
        print(f" >> Target Platform: {target.upper()}")
        print(f" >> Flutter Project: {proj_dir}")
        print(f" >> Compiled Flutter/Dart: {main_dart_path}")

        import shutil
        import subprocess
        flutter_bin = shutil.which("flutter")
        if flutter_bin:
            print(f" >> Invoking Flutter SDK: flutter build {target}...")
            build_cmd = [flutter_bin, "build", target]
            res = subprocess.run(build_cmd, cwd=proj_dir)
            if res.returncode == 0:
                print(f"[SUCCESS] Production {target.upper()} package generated successfully.")
            else:
                print(f"[ERROR] Flutter build failed with exit code {res.returncode}", file=sys.stderr)
                sys.exit(res.returncode)
        else:
            print(f"[INFO] Flutter SDK was not found on PATH.")
            print(f"To compile the final {target.upper()} binary, install Flutter (https://flutter.dev) and run:")
            print(f"  cd {proj_dir} && flutter build {target}")
        sys.exit(0)

    src = args[0]
    out = None
    if "-o" in args:
        idx = args.index("-o")
        if idx + 1 < len(args):
            out = args[idx + 1]

    from enlgm.compiler import build_enlgm_file
    build_enlgm_file(src, out)

if __name__ == "__main__":
    main()
