import os
import shutil
import stat

def remove_readonly(func, path, excinfo):
    try:
        os.chmod(path, stat.S_IWRITE)
        func(path)
    except Exception as e:
        print(f"Failed to remove {path}: {e}")

targets_to_delete = [
    r"D:\Enlang",
    r"D:\enlang demo",
    r"D:\enlang ecosystem",
    r"D:\enlang stress test",
    r"D:\enlang test",
    r"D:\enlangg (2).zip",
    r"D:\enlangg archieve",
    r"D:\enlangg.zip",
    r"D:\TheEnlangGranthVolumeXXIX.tex (1).pdf",
    r"C:\Users\spand\.enlang",
]

for target in targets_to_delete:
    if os.path.exists(target):
        try:
            if os.path.isdir(target):
                shutil.rmtree(target, onexc=remove_readonly)
                print(f"[DELETED FOLDER] {target}")
            else:
                os.remove(target)
                print(f"[DELETED FILE]   {target}")
        except Exception as e:
            print(f"[ERROR DELETING] {target}: {e}")
    else:
        print(f"[NOT FOUND]      {target}")

print("Cleanup complete!")
