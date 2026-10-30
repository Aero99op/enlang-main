#!/bin/sh
# Test enlangg-install.sh against a real build: install, PATH edits, upgrade, uninstall.
#   sh tests/test_posix_installer.sh /path/to/enlangg-install.sh EXPECTED_VERSION
# Uses a throwaway HOME, so it never touches your real shell files.
set -u
INST=$1; VER=$2
INST=$(cd "$(dirname "$INST")" && pwd)/$(basename "$INST")
T=$(mktemp -d "${TMPDIR:-/tmp}/enlangg-posixtest.XXXXXX")
trap 'rm -rf "$T"' EXIT
pass=0; fail=0
check() { if [ "$2" = 0 ]; then pass=$((pass + 1)); echo "PASS  $1"; else fail=$((fail + 1)); echo "FAIL  $1"; fi; }
ok()   { check "$1" 0; }
yes_if() { desc=$1; shift; if "$@"; then check "$desc" 0; else check "$desc" 1; fi; }
no_if()  { desc=$1; shift; if "$@"; then check "$desc" 1; else check "$desc" 0; fi; }

export HOME="$T/home"; mkdir -p "$HOME"
P="$HOME/enlangg-test"
unset ENLANGG_HOME ENLANGG_INSTALL_TARGET

printf '# my bashrc\nalias ll="ls -l"\n' > "$HOME/.bashrc"
printf 'export FOO=1\n' > "$HOME/.profile"
cp "$HOME/.bashrc" "$T/bashrc.orig"; cp "$HOME/.profile" "$T/profile.orig"

# --- refuses bad folders
no_if "reject /"              sh "$INST" --prefix /
no_if "reject relative"       sh "$INST" --prefix foo/bar
no_if "reject .."             sh "$INST" --prefix "$T/a/../b"
no_if "reject HOME itself"    sh "$INST" --prefix "$HOME"
no_if "reject quote chars"    sh "$INST" --prefix "$T/we\"ird"
no_if "reject top-level dir"  sh "$INST" --prefix /usr
no_if "unknown option"        sh "$INST" --bogus
no_if "refuses piped stdin"   sh -c "cat '$INST' | sh -s -- --prefix '$P'"
yes_if "nothing written yet"  test ! -e "$P"

# --- install
sh "$INST" --prefix "$P" > "$T/out.txt" 2>&1; rc=$?
check "install exit 0" "$rc"
cat "$T/out.txt" | sed 's/^/    | /'
for t in enlangg enlng enlngdb enlngf enlngd enlngs enlngm; do
    yes_if "bin/$t is executable" test -x "$P/bin/$t"
done
for f in lib/lib_std.enlng lib/lib_math.enlng include/enlng_runtime.h LICENSE uninstall.sh install.manifest; do
    yes_if "file $f" test -e "$P/$f"
done
yes_if "manifest header"     test "$(sed -n 1p "$P/install.manifest")" = "enlangg-install-manifest 1"
yes_if "manifest lists enlng" grep -qx "bin/enlng" "$P/install.manifest"
yes_if ".bashrc has block"   grep -qF "# >>> enlangg >>>" "$HOME/.bashrc"
yes_if ".profile has block"  grep -qF "# >>> enlangg >>>" "$HOME/.profile"
yes_if ".bashrc keeps own lines" grep -qF 'alias ll="ls -l"' "$HOME/.bashrc"
v=$("$P/bin/enlng" --version 2>&1)
yes_if "enlng --version $VER" test "$(echo "$v" | grep -cF "$VER")" -ge 1

# PATH from the new block actually resolves the tool
res=$(env -i HOME="$HOME" PATH=/usr/bin:/bin sh -c '. "$HOME/.bashrc"; command -v enlangg')
want=$(printf '%s' "$P/bin/enlangg" | sed 's|//*|/|g')
yes_if "sourcing .bashrc puts enlangg on PATH" test "$res" = "$want"

# the installed toolchain works from a clean directory
mkdir -p "$T/work"; cd "$T/work" || exit 1
printf 'type enlng\nuse "lib_math.enlng"\nshow square(12)\n' > smoke.enlng
# no C compiler is reachable here (PATH is empty), so this proves the built-in interpreter runs it
out=$(env -i HOME="$HOME" PATH=/nonexistent "$P/bin/enlangg" run smoke.enlng 2>&1)
yes_if "enlangg run smoke.enlng -> 144 with no compiler on PATH" test "$out" = "144"

# --- reinstall is idempotent, removes stale files
echo x > "$P/lib/stale.enlng"; echo "lib/stale.enlng" >> "$P/install.manifest"
sh "$INST" --prefix "$P" >/dev/null 2>&1; check "reinstall exit 0" $?
yes_if "one .bashrc block"   test "$(grep -cF '# >>> enlangg >>>' "$HOME/.bashrc")" = 1
yes_if "stale file removed"  test ! -e "$P/lib/stale.enlng"

# --- user files survive uninstall
echo keep > "$P/mine.txt"
sh "$P/uninstall.sh" >/dev/null 2>&1; check "uninstall exit 0" $?
yes_if "bin removed"         test ! -e "$P/bin"
yes_if "lib removed"         test ! -e "$P/lib"
yes_if "manifest removed"    test ! -e "$P/install.manifest"
yes_if "user file kept"      test -f "$P/mine.txt"
yes_if ".bashrc restored exactly"  cmp -s "$HOME/.bashrc" "$T/bashrc.orig"
yes_if ".profile restored exactly" cmp -s "$HOME/.profile" "$T/profile.orig"
rm -rf "$P"

# --- --no-path leaves startup files alone
sh "$INST" --prefix "$P" --no-path >/dev/null 2>&1; check "--no-path install exit 0" $?
yes_if "--no-path: .bashrc untouched" cmp -s "$HOME/.bashrc" "$T/bashrc.orig"
sh "$INST" --prefix "$P" --uninstall >/dev/null 2>&1; check "--uninstall via installer" $?
yes_if "folder gone when empty" test ! -e "$P"

# --- a fresh HOME with no startup files gets a .profile, removed again on uninstall
rm -f "$HOME/.bashrc" "$HOME/.profile"
SHELL=/bin/sh sh "$INST" --prefix "$P" >/dev/null 2>&1; check "install with no rc files" $?
yes_if "created .profile"    grep -qF "# >>> enlangg >>>" "$HOME/.profile"
sh "$P/uninstall.sh" >/dev/null 2>&1
yes_if "empty .profile removed" test ! -e "$HOME/.profile"

# --- fish
mkdir -p "$HOME/.config/fish"
sh "$INST" --prefix "$P" >/dev/null 2>&1
yes_if "fish conf.d file"    test -f "$HOME/.config/fish/conf.d/enlangg.fish"
sh "$P/uninstall.sh" >/dev/null 2>&1
yes_if "fish file removed"   test ! -e "$HOME/.config/fish/conf.d/enlangg.fish"

# --- uninstall refuses a folder that is not ours
mkdir -p "$T/victim"; echo x > "$T/victim/file.txt"
printf 'file.txt\n' > "$T/victim/install.manifest"
cp "$INST" "$T/victim/uninstall.sh"
sh "$T/victim/uninstall.sh" >/dev/null 2>&1
yes_if "unregistered folder untouched" test -f "$T/victim/file.txt"

# --- a damaged installer is rejected before anything is written
cp "$INST" "$T/bad.sh"
line=$(awk '/^__ENLANGG_PAYLOAD__$/ { print NR; exit }' "$T/bad.sh")
base=$(head -n "$line" "$T/bad.sh" | wc -c | tr -d ' ')
# flip one byte inside every platform's archive, so whichever one this machine uses is damaged
for off in $(awk '$1 == "#@PAYLOAD" { print $3 }' "$T/bad.sh"); do
    printf 'X' | dd of="$T/bad.sh" bs=1 seek=$((base + off + 500)) conv=notrunc 2>/dev/null
done
no_if "corrupted installer rejected" sh "$T/bad.sh" --prefix "$T/never"
yes_if "corrupted: nothing written" test ! -e "$T/never"

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
