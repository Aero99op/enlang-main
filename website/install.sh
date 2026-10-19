#!/bin/sh
# =====================================================================
#   Enlangg Sovereign Toolchain - Linux & macOS Universal Installer
#   Pure C Zero-Python 7-in-1 Sovereign Suite
#   Usage:
#     curl -fsSL https://enlangg.vercel.app/install.sh | bash
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

INSTALL_DIR="$HOME/.enlangg/bin"
mkdir -p "$INSTALL_DIR"

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

build_from_source() {
    SRC_ROOT="$1"
    echo "   Compiling pure C sovereign core engines from '$SRC_ROOT' using $CC..."

    echo "   [1/7] Compiling enlangg (Universal Dispatcher)..."
    $CC -O2 -I"$SRC_ROOT" -I"$SRC_ROOT/enlngdb/c" -I"$SRC_ROOT/enlng/c" \
        "$SRC_ROOT/enlangg.c" "$SRC_ROOT/enlngdb/c/enlngdb.c" "$SRC_ROOT/enlngdb/c/enlngdb_parser.c" "$SRC_ROOT/enlng/c/enlng_emitter.c" \
        -lm -o "$INSTALL_DIR/enlangg"

    echo "   [2/7] Compiling enlng (Core Language)..."
    $CC -O2 -I"$SRC_ROOT" -I"$SRC_ROOT/src" \
        "$SRC_ROOT/enlng.c" -lm -o "$INSTALL_DIR/enlng"

    echo "   [3/7] Compiling enlngdb (Database Engine)..."
    $CC -O2 -I"$SRC_ROOT/enlngdb/c" \
        "$SRC_ROOT/enlngdb/c/main.c" "$SRC_ROOT/enlngdb/c/enlngdb.c" "$SRC_ROOT/enlngdb/c/enlngdb_parser.c" \
        -lm -o "$INSTALL_DIR/enlngdb"

    echo "   [4/7] Compiling enlngf (Frontend Studio)..."
    $CC -O2 "$SRC_ROOT/enlngf.c" -o "$INSTALL_DIR/enlngf"

    echo "   [5/7] Compiling enlngs (Reactive Script Engine)..."
    $CC -O2 "$SRC_ROOT/enlngs.c" -o "$INSTALL_DIR/enlngs"

    echo "   [6/7] Compiling enlngd (Design Tokens Engine)..."
    $CC -O2 "$SRC_ROOT/enlngd.c" -o "$INSTALL_DIR/enlngd"

    echo "   [7/7] Compiling enlngm (Mobile HAL Packager)..."
    $CC -O2 "$SRC_ROOT/enlngm.c" -o "$INSTALL_DIR/enlngm"
}

# 1. Install via Local Source, Release Tarball, or Git Clone
if [ -f "$SCRIPT_DIR/enlangg.c" ] && [ -n "$CC" ]; then
    build_from_source "$SCRIPT_DIR"
elif curl -f -s -I "$RELEASE_URL" >/dev/null 2>&1; then
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
    tar -xzf "$TMP_DIR/$ARCHIVE_NAME" -C "$INSTALL_DIR"
    rm -rf "$TMP_DIR"
elif [ -n "$CC" ] && command -v git >/dev/null 2>&1; then
    echo "   Cloning official repository to build from source..."
    TMP_CLONE="/tmp/enlang-build-$$"
    git clone --depth 1 "https://github.com/$GITHUB_REPO.git" "$TMP_CLONE"
    build_from_source "$TMP_CLONE"
    rm -rf "$TMP_CLONE"
else
    echo "${RED}[ERROR] Neither a C compiler (gcc/clang) nor a prebuilt release archive could be accessed.${NC}"
    echo "Please install gcc or clang and run the installer again."
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
    fi
done

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
echo "Website & Live Playground: https://enlangg.vercel.app"
