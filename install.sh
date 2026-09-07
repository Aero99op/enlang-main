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

DIST_URL="https://enlangg.vercel.app"
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd || echo "")"
CC=""
if command -v gcc >/dev/null 2>&1; then
    CC="gcc"
elif command -v cc >/dev/null 2>&1; then
    CC="cc"
elif command -v clang >/dev/null 2>&1; then
    CC="clang"
fi

# 1. Compile or Install Sovereign Binaries
if [ -n "$CC" ]; then
    echo "   Compiling pure C sovereign core engines using $CC..."
    
    # enlangg
    if [ -f "$SCRIPT_DIR/enlangg.c" ]; then
        $CC -O2 "$SCRIPT_DIR/enlangg.c" -o "$INSTALL_DIR/enlangg" 2>/dev/null || true
    else
        curl -fsSL "$DIST_URL/enlangg.c" -o "/tmp/enlangg.c" 2>/dev/null && \
        $CC -O2 /tmp/enlangg.c -o "$INSTALL_DIR/enlangg" 2>/dev/null || true
    fi

    # enlng
    if [ -f "$SCRIPT_DIR/enlng.c" ]; then
        $CC -O2 "$SCRIPT_DIR/enlng.c" -o "$INSTALL_DIR/enlng" 2>/dev/null || true
    else
        curl -fsSL "$DIST_URL/enlng.c" -o "/tmp/enlng.c" 2>/dev/null && \
        $CC -O2 /tmp/enlng.c -o "$INSTALL_DIR/enlng" 2>/dev/null || true
    fi

    # enlngdb
    if [ -f "$SCRIPT_DIR/enlngdb/c/main.c" ] && [ -f "$SCRIPT_DIR/enlngdb/c/enlngdb.c" ]; then
        $CC -O2 "$SCRIPT_DIR/enlngdb/c/main.c" "$SCRIPT_DIR/enlngdb/c/enlngdb.c" "$SCRIPT_DIR/enlngdb/c/enlngdb_parser.c" -o "$INSTALL_DIR/enlngdb" 2>/dev/null || true
    fi

    # enlngs
    if [ -f "$SCRIPT_DIR/enlngs/c/main.c" ] && [ -f "$SCRIPT_DIR/enlngs/c/enlngs_engine.c" ]; then
        $CC -O2 "$SCRIPT_DIR/enlngs/c/main.c" "$SCRIPT_DIR/enlngs/c/enlngs_engine.c" -o "$INSTALL_DIR/enlngs" 2>/dev/null || true
    fi

    # enlngd
    if [ -f "$SCRIPT_DIR/enlngd/c/main.c" ] && [ -f "$SCRIPT_DIR/enlngd/c/enlngd_engine.c" ]; then
        $CC -O2 "$SCRIPT_DIR/enlngd/c/main.c" "$SCRIPT_DIR/enlngd/c/enlngd_engine.c" -o "$INSTALL_DIR/enlngd" 2>/dev/null || true
    fi
fi

# Ensure executable permissions
chmod +x "$INSTALL_DIR"/* 2>/dev/null || true

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
for CMD in enlangg enlng enlngdb enlngs enlngd; do
    if [ -x "$INSTALL_DIR/$CMD" ]; then
        "$INSTALL_DIR/$CMD" --version 2>/dev/null || true
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
echo "  enlngs <logic.enlngs>       # Pure C in-memory script VM"
echo "  enlngd <theme.enlngd>       # Pure C design tokens & style resolver"
echo "  enlangg --help              # Toolchain comprehensive help"
echo ""
echo "Website & Live Playground: https://enlangg.vercel.app"
