#!/bin/sh
# =====================================================================
#   Enlangg Sovereign Toolchain - Linux & macOS Universal Installer
#   Pure C Zero-Python 7-in-1 Sovereign Suite
#   Usage:
#     curl -fsSL https://enlangg.site/install.sh | bash
# =====================================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m' # No Color

printf "${CYAN}${BOLD}"
cat << 'EOF'
=====================================================================
    ENLANGG SOVEREIGN 7-IN-1 SUITE - Linux & macOS Universal Installer
    Pure C Zero-Python Native Architecture
=====================================================================
EOF
printf "${NC}\n"

INSTALL_ROOT="$HOME/.enlangg"
INSTALL_DIR="$INSTALL_ROOT/bin"
mkdir -p "$INSTALL_DIR" "$INSTALL_ROOT/lib" "$INSTALL_ROOT/include"

OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

case "$ARCH" in
    x86_64|amd64) ARCH="amd64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    *) ARCH="amd64" ;;
esac

echo "${YELLOW}>> Target Platform: $OS ($ARCH)${NC}"
echo "${YELLOW}>> Installing 7 Sovereign Executables to: $INSTALL_DIR ...${NC}"

GITHUB_REPO="Aero99op/enlang-main"
ARCHIVE_NAME="enlangg-${OS}-${ARCH}.tar.gz"
RELEASE_BASE="https://github.com/$GITHUB_REPO/releases/latest/download"
RELEASE_URL="$RELEASE_BASE/$ARCHIVE_NAME"
SUMS_URL="$RELEASE_BASE/SHA256SUMS"
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd || echo "")"

CC=""
if command -v gcc >/dev/null 2>&1; then
    CC="gcc"
elif command -v clang >/dev/null 2>&1; then
    CC="clang"
elif command -v cc >/dev/null 2>&1; then
    CC="cc"
fi

# Builds the seven tools from the source tree in $1 (about a minute), and copies the standard library and headers.
build_from_source() {
    SRC_ROOT="$1"
    echo "   Compiling the 7 tools from '$SRC_ROOT' using $CC (about a minute)..."
    (
        cd "$SRC_ROOT" || exit 1
        set -e
        $CC -O2 -I. -Isrc -Ienlngdb/c -Ienlng/c enlangg.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlng/c/enlng_emitter.c -lm -o "$INSTALL_DIR/enlangg"
        $CC -O2 -I. -Isrc enlng.c -lm -pthread -o "$INSTALL_DIR/enlng"
        $CC -O2 -I. -Isrc -Ienlngdb/c enlngdb/c/main.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c -lm -o "$INSTALL_DIR/enlngdb"
        $CC -O2 -I. -Isrc -Ienlngf/c enlngf.c -lm -o "$INSTALL_DIR/enlngf"
        $CC -O2 -I. -Isrc -Ienlngs/c enlngs.c -lm -o "$INSTALL_DIR/enlngs"
        $CC -O2 -I. -Isrc -Ienlngd/c enlngd.c -lm -o "$INSTALL_DIR/enlngd"
        $CC -O2 -I. -Isrc -Ienlngm/c enlngm.c -lm -o "$INSTALL_DIR/enlngm"
        cp lib_*.enlng "$INSTALL_ROOT/lib/"
        cp src/enlng_runtime.h enlng/c/enlng_mem.h "$INSTALL_ROOT/include/"
    ) || {
        echo "${RED}Error: the build failed (see the compiler's message above). Nothing was added to your PATH.${NC}" >&2
        exit 1
    }
}

# 1. Install via Local Source, Release Tarball, or Git Clone
if [ -f "$SCRIPT_DIR/enlangg.c" ] && [ -n "$CC" ]; then
    build_from_source "$SCRIPT_DIR"
elif curl -fsIL "$RELEASE_URL" >/dev/null 2>&1 && curl -fsIL "$SUMS_URL" >/dev/null 2>&1; then
    echo "   Downloading prebuilt release archive and SHA256SUMS from GitHub..."
    TMP_DIR="$(mktemp -d 2>/dev/null || mktemp -d -t 'enlangg')"
    if ! curl -fsSL "$SUMS_URL" -o "$TMP_DIR/SHA256SUMS"; then
        echo "${RED}Error: Failed to download SHA256SUMS from $SUMS_URL${NC}" >&2
        rm -rf "$TMP_DIR"
        exit 1
    fi
    if ! curl -fsSL "$RELEASE_URL" -o "$TMP_DIR/$ARCHIVE_NAME"; then
        echo "${RED}Error: Failed to download archive from $RELEASE_URL${NC}" >&2
        rm -rf "$TMP_DIR"
        exit 1
    fi

    EXPECTED_SHA=$(grep "[[:space:]]${ARCHIVE_NAME}$" "$TMP_DIR/SHA256SUMS" | awk '{print $1}')
    if [ -z "$EXPECTED_SHA" ]; then
        echo "${RED}Error: Could not locate checksum for $ARCHIVE_NAME in SHA256SUMS.${NC}" >&2
        rm -rf "$TMP_DIR"
        exit 1
    fi

    if command -v sha256sum >/dev/null 2>&1; then
        ACTUAL_SHA=$(sha256sum "$TMP_DIR/$ARCHIVE_NAME" | awk '{print $1}')
    elif command -v shasum >/dev/null 2>&1; then
        ACTUAL_SHA=$(shasum -a 256 "$TMP_DIR/$ARCHIVE_NAME" | awk '{print $1}')
    else
        echo "${RED}Error: Neither sha256sum nor shasum is available for verification.${NC}" >&2
        rm -rf "$TMP_DIR"
        exit 1
    fi

    if [ "$ACTUAL_SHA" != "$EXPECTED_SHA" ]; then
        echo "${RED}Error: SHA-256 verification failed!${NC}" >&2
        echo "  Expected: $EXPECTED_SHA" >&2
        echo "  Actual:   $ACTUAL_SHA" >&2
        rm -rf "$TMP_DIR"
        exit 1
    fi
    echo "   ✔ Verified SHA-256: $ACTUAL_SHA"
    tar -xzf "$TMP_DIR/$ARCHIVE_NAME" -C "$INSTALL_ROOT"
    rm -rf "$TMP_DIR"
elif [ -n "$CC" ]; then
    # no prebuilt archive for this platform yet: the source is fetched and built here
    echo "   No prebuilt archive for $OS ($ARCH) yet: fetching the source to build it here..."
    TMP_SRC="$(mktemp -d 2>/dev/null || mktemp -d -t 'enlangg')"
    if ! curl -fsSL "https://github.com/$GITHUB_REPO/archive/refs/heads/main.tar.gz" -o "$TMP_SRC/src.tar.gz" || ! tar -xzf "$TMP_SRC/src.tar.gz" -C "$TMP_SRC"; then
        echo "${RED}Error: the source could not be downloaded from github.com/$GITHUB_REPO.${NC}" >&2
        rm -rf "$TMP_SRC"
        exit 1
    fi
    SRC_DIR="$(find "$TMP_SRC" -maxdepth 2 -name enlangg.c | head -n 1)"
    if [ -z "$SRC_DIR" ]; then
        echo "${RED}Error: the downloaded source has no enlangg.c.${NC}" >&2
        rm -rf "$TMP_SRC"
        exit 1
    fi
    build_from_source "$(dirname "$SRC_DIR")"
    rm -rf "$TMP_SRC"
else
    echo "${RED}[ERROR] There is no prebuilt archive for $OS ($ARCH) yet, and no C compiler (gcc, clang or cc) to build from source.${NC}"
    echo "Install one and run the installer again:"
    echo "  Debian/Ubuntu: sudo apt install build-essential     macOS: xcode-select --install"
    exit 1
fi

# Ensure executable permissions
chmod +x "$INSTALL_DIR"/* 2>/dev/null

# 2. Update Shell Profiles (PATH)
echo "${YELLOW}>> Configuring system PATH environment variable...${NC}"

SHELL_PROFILES="$HOME/.bashrc $HOME/.zshrc $HOME/.profile $HOME/.bash_profile"
PATH_LINE="export PATH=\"\$HOME/.enlangg/bin:\$PATH\""
UPDATED=0

for PROFILE in $SHELL_PROFILES; do
    if [ -f "$PROFILE" ]; then
        if ! grep -q ".enlangg/bin" "$PROFILE"; then
            echo "" >> "$PROFILE"
            echo "# Enlangg Sovereign Toolchain" >> "$PROFILE"
            echo "$PATH_LINE" >> "$PROFILE"
            echo "   [OK] Added PATH to $PROFILE"
            UPDATED=1
        fi
    fi
done

if [ "$UPDATED" -eq 0 ]; then
    if [ -f "$HOME/.profile" ]; then
        echo "$PATH_LINE" >> "$HOME/.profile"
    else
        echo "$PATH_LINE" >> "$HOME/.bashrc"
    fi
fi

export PATH="$INSTALL_DIR:$PATH"

# 3. Verification
echo "${GREEN}>> Verifying installed tools:${NC}"
for CMD in enlangg enlng enlngdb enlngf enlngs enlngd enlngm; do
    if [ -x "$INSTALL_DIR/$CMD" ]; then
        printf "   %-10s: " "$CMD"
        "$INSTALL_DIR/$CMD" --version
    else
        echo "${RED}   [MISSING] $CMD failed to install.${NC}"
        MISSING=1
    fi
done
if [ -n "$MISSING" ]; then
    echo "${RED}The install is not complete: the tools marked [MISSING] are not there.${NC}" >&2
    exit 1
fi

printf "${GREEN}${BOLD}"
cat << 'EOF'
=====================================================================
  [SUCCESS] Enlangg Sovereign Pure C Suite installed successfully! 🚀
=====================================================================
EOF
printf "${NC}\n"
echo "To start using immediately in this session, run:"
echo "  export PATH=\"\$HOME/.enlangg/bin:\$PATH\""
echo ""
echo "Available Pure C Sovereign Commands:"
echo "  enlangg <file>              # Universal toolchain dispatcher"
echo "  enlng run <file.enlng>      # Core backend language engine"
echo "  enlngdb <script.enlngdb>    # Pure C microsecond database"
echo "  enlngf <app.enlngf>         # Pure C frontend studio"
echo "  enlngs <logic.enlngs>       # Pure C in-memory script VM"
echo "  enlngd <theme.enlngd>       # Pure C design tokens & style resolver"
echo "  enlngm <app.enlngm>         # Pure C smartphone simulator & HAL"
echo "  enlangg --help              # Toolchain comprehensive help"
echo ""
echo "Website & Live Playground: https://enlangg.site"
