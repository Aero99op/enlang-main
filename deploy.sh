#!/bin/bash
set -e

echo "==================================================="
echo "  Enlangg Official Website - Instant Vercel Deploy"
echo "==================================================="

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/website"

if [ ! -d "$SRC_DIR" ]; then
    echo "[ERROR] Website directory not found at $SRC_DIR"
    exit 1
fi

TMP_DEPLOY="/tmp/enlangg_deploy_$$"

echo "[1/3] Preparing clean deployment bundle..."
rm -rf "$TMP_DEPLOY" 2>/dev/null || true
mkdir -p "$TMP_DEPLOY"
cp -r "$SRC_DIR"/* "$TMP_DEPLOY"/
if [ -d "$SRC_DIR/.vercel" ]; then
    cp -r "$SRC_DIR/.vercel" "$TMP_DEPLOY"/
fi

echo "[2/3] Uploading and deploying to Vercel Production..."
cd "$TMP_DEPLOY"
npx --yes vercel --prod --yes
DEPLOY_EXIT=$?
cd "$SCRIPT_DIR"

echo "[3/3] Cleaning up temporary workspace..."
rm -rf "$TMP_DEPLOY" 2>/dev/null || true

if [ $DEPLOY_EXIT -eq 0 ]; then
    echo "==================================================="
    echo " [SUCCESS] Live at https://enlangg.vercel.app"
    echo "==================================================="
else
    echo "==================================================="
    echo " [FAILED] Deployment exited with code $DEPLOY_EXIT"
    echo "==================================================="
fi

exit $DEPLOY_EXIT
