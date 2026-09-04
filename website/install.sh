#!/bin/sh
# =====================================================================
#   Enlangg Sovereign Toolchain - Linux & macOS Universal Installer
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
    ENLANGG & ENLNG - Sovereign Programming Language Toolchain
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
echo "${YELLOW}>> Installing to: $INSTALL_DIR ...${NC}"

DIST_URL="https://enlangg.vercel.app"

# 1. Compile or Download Binaries
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd || echo "")"
if [ -f "$SCRIPT_DIR/enlangg.c" ] && [ -f "$SCRIPT_DIR/enlng.c" ] && command -v gcc >/dev/null 2>&1; then
    echo "   Compiling directly from local source using gcc..."
    gcc -O2 "$SCRIPT_DIR/enlangg.c" -o "$INSTALL_DIR/enlangg"
    gcc -O2 "$SCRIPT_DIR/enlng.c" -o "$INSTALL_DIR/enlng"
    chmod +x "$INSTALL_DIR/enlangg" "$INSTALL_DIR/enlng"
else
    # Compile from sovereign website distribution source
    if command -v cc >/dev/null 2>&1 || command -v gcc >/dev/null 2>&1; then
        CC="$(command -v cc || command -v gcc)"
        echo "   Compiling on target machine using $CC..."
        curl -fsSL "$DIST_URL/enlangg.c" -o "/tmp/enlangg.c"
        curl -fsSL "$DIST_URL/enlng.c" -o "/tmp/enlng.c"
        $CC -O2 /tmp/enlangg.c -o "$INSTALL_DIR/enlangg"
        $CC -O2 /tmp/enlng.c -o "$INSTALL_DIR/enlng"
        chmod +x "$INSTALL_DIR/enlangg" "$INSTALL_DIR/enlng"
    fi
fi
chmod +x "$INSTALL_DIR/enlangg" "$INSTALL_DIR/enlng" 2>/dev/null || true

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
echo "${GREEN}>> Verifying installation:${NC}"
if [ -x "$INSTALL_DIR/enlangg" ]; then
    "$INSTALL_DIR/enlangg" --version || true
fi
if [ -x "$INSTALL_DIR/enlng" ]; then
    "$INSTALL_DIR/enlng" --version || true
fi

printf "${GREEN}${BOLD}"
cat << 'EOF'
=====================================================================
  [SUCCESS] Enlangg & Enlng installed successfully! 🚀
=====================================================================
EOF
printf "${NC}\n"
echo "To start using immediately in this session, run:"
echo "  export PATH=\"\$HOME/.enlangg/bin:\$PATH\""
echo ""
echo "Commands:"
echo "  enlangg run <file.enlng>   # Execute natural backend code"
echo "  enlng run <file.enlng>     # Pure sovereign general-purpose engine"
echo "  enlangg --help             # Toolchain help"
echo ""
echo "Website & Live Playground: https://enlangg.vercel.app"
