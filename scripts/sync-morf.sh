#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
base="${MORF_SRC_BASE:-$(cd "$root/.." && pwd)}"
source="$base/morfDeploy"
[ -d "$source/morfdeploy" ] || source="$base/morfDeploy_travail"
destination="$root/third_party/morf/morfdeploy"

[ -d "$source/morfdeploy" ] || { echo "morfDeploy source is missing" >&2; exit 1; }
case "$destination" in "$root"/*) ;; *) echo "refusing unsafe destination" >&2; exit 1;; esac
rm -rf "$destination"
mkdir -p "$destination"
cp -a "$source/morfdeploy/." "$destination/"
cp "$source/VERSION" "$destination/VERSION"
find "$destination" -type d -name __pycache__ -prune -exec rm -rf {} +
echo "morfdeploy synchronized."
