#!/bin/sh
# Build enlangg-install.sh: the shell installer with each platform's archive appended.
#
#   make_installer.sh VERSION OUT key=archive.tar.gz [key=archive.tar.gz ...]
#
# Keys must match what the installer detects: linux-x86_64, linux-aarch64, macos.
set -eu

[ $# -ge 3 ] || { echo "usage: $0 VERSION OUT key=archive ..." >&2; exit 2; }
here=$(cd "$(dirname "$0")" && pwd)
version=$1; out=$2; shift 2

sha256() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | awk '{ print $1 }'
    else shasum -a 256 "$1" | awk '{ print $1 }'
    fi
}

work=$(mktemp -d "${TMPDIR:-/tmp}/enlangg-pack.XXXXXX")
trap 'rm -rf "$work"' EXIT

off=0
: > "$work/table"
for spec in "$@"; do
    key=${spec%%=*}; file=${spec#*=}
    [ -f "$file" ] || { echo "missing archive for $key: $file" >&2; exit 1; }
    size=$(wc -c < "$file" | tr -d ' ')
    printf '#@PAYLOAD %s %s %s %s\n' "$key" "$off" "$size" "$(sha256 "$file")" >> "$work/table"
    off=$((off + size))
done

tr -d '\r' < "$here/enlangg-install.sh.in" | sed "s/@VERSION@/$version/g" |
    awk -v tf="$work/table" '$0 == "#@TABLE@" { while ((getline l < tf) > 0) print l; next } { print }' > "$work/head"

cat "$work/head" > "$out"
for spec in "$@"; do cat "${spec#*=}" >> "$out"; done
chmod 755 "$out"
echo "wrote $out ($(wc -c < "$out" | tr -d ' ') bytes)"
