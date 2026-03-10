#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.2.0"

if git -C "$SCRIPT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
	BRANCH="$(git -C "$SCRIPT_DIR" branch --show-current | tr -d '\r\n')"
	COMMIT="$(git -C "$SCRIPT_DIR" rev-parse --short HEAD | tr -d '\r\n')"
else
	BRANCH="unknown"
	COMMIT="unknown"
fi

DATE="$(date '+%A, %B %d, %Y %I:%M:%S %p' | sed 's/^0//; s/ 0/ /g')"

echo "Branch: $BRANCH"
echo "Commit: $COMMIT"
echo "Built: $DATE"

cat > "$SCRIPT_DIR/version.h" <<EOF
char Version[] = "$VERSION";
char GitBranch[] = "$BRANCH";
char GitCommit[] = "$COMMIT";
char BuildDate[] = "$DATE";

EOF