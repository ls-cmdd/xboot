#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

VERSION=$(git rev-parse --short=12 HEAD)
EPOCH=$(git show -s --format=%ct HEAD)
NAME="xboot-release-${VERSION}"
OUT="$ROOT/release/$NAME"
ARCHIVE="$ROOT/release/${NAME}.zip"

rm -rf "$OUT" "$ARCHIVE"
mkdir -p "$OUT/source" "$OUT/deploy" "$OUT/docs"

git archive --format=tar HEAD | tar -xf - -C "$OUT/source"
mkdir -p "$OUT/source/assets"
cp -a assets/. "$OUT/source/assets/"
cp build/xboot.efi "$OUT/deploy/BOOTX64.EFI"
cp build/xboot-esp.img "$OUT/deploy/xboot-esp.img"
cp config/xboot.conf "$OUT/deploy/xboot.conf"
cp config/xboot-hash.conf "$OUT/deploy/xboot-hash.conf"
cp docs/RELEASE_PACKAGE_README.md "$OUT/README.md"
cp docs/RELEASE_EVIDENCE.md docs/STATUS.md docs/TESTING.md \
   docs/COMPATIBILITY.md docs/REQUIREMENTS_AUDIT.md "$OUT/docs/"

{
    printf 'XBoot release %s\n' "$VERSION"
    printf 'Source commit: %s\n' "$(git rev-parse HEAD)"
    printf 'Source date epoch: %s\n' "$EPOCH"
    printf 'Generated at UTC: %s\n\n' "$(date -u -d "@$EPOCH" '+%Y-%m-%dT%H:%M:%SZ')"
    printf 'Contents:\n'
    find "$OUT" -type f -printf '%P\n' | LC_ALL=C sort
} > "$OUT/RELEASE-METADATA.txt"

(
    cd "$OUT"
    find . -type f ! -name MANIFEST.sha256 -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > MANIFEST.sha256
)

mkdir -p "$ROOT/release"
export TZ=UTC
find "$OUT" -exec touch --date="@$EPOCH" {} +
(
    cd "$ROOT/release"
    zip -X -9 -r "$ARCHIVE" "$NAME" >/dev/null
)
sha256sum "$ARCHIVE" > "${ARCHIVE}.sha256"
printf '%s\n' "$ARCHIVE"
printf '%s\n' "${ARCHIVE}.sha256"
