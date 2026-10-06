#!/usr/bin/env bash
set -euo pipefail

TAG=${1:?usage: ./scripts/release.sh v0.3.1-beta}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

[[ -d .git ]] || { echo "release: Git repository required" >&2; exit 1; }
[[ -z "$(git status --porcelain)" ]] || { echo "release: working tree must be clean" >&2; exit 1; }
git rev-parse "$TAG" >/dev/null 2>&1 || { echo "release: tag $TAG does not exist" >&2; exit 1; }
VERSION=$(cat VERSION)
EXPECTED="v${VERSION#beta-}-beta"
[[ "$TAG" == "$EXPECTED" ]] || { echo "release: tag $TAG does not match VERSION=$VERSION (expected $EXPECTED)" >&2; exit 1; }

mkdir -p dist
ARCHIVE="dist/balance-${VERSION}.tar.gz"
CHECKSUM="dist/balance-${VERSION}.sha256"
git archive --format=tar.gz --prefix="balance-${VERSION}/" "$TAG" -o "$ARCHIVE"
shasum -a 256 "$ARCHIVE" > "$CHECKSUM"
printf 'release: %s\nchecksum: %s\n' "$ARCHIVE" "$CHECKSUM"
