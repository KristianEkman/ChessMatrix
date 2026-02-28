#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

NEW_ENGINE="${1:-$ROOT_DIR/chessmatrix}"
OLD_ENGINE="${2:-$ROOT_DIR/chessmatrix}"
GAMES="${3:-400}"
TC="${4:-3+0.03}"
CONCURRENCY="${5:-1}"
OPENINGS_FILE="${6:-$ROOT_DIR/openings.epd}"
CUTECHESS_HINT="${7:-${CUTECHESS_BIN:-}}"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="$ROOT_DIR/bench/$TIMESTAMP"
mkdir -p "$OUT_DIR"

if [[ -n "$CUTECHESS_HINT" && -x "$CUTECHESS_HINT" ]]; then
  CUTECHESS="$CUTECHESS_HINT"
elif [[ -x "$ROOT_DIR/cutechess-cli" ]]; then
  CUTECHESS="$ROOT_DIR/cutechess-cli"
elif command -v cutechess-cli >/dev/null 2>&1; then
  CUTECHESS="$(command -v cutechess-cli)"
else
  echo "Error: cutechess-cli not found. Put it at ./cutechess-cli or install it in PATH."
  exit 1
fi

if [[ ! -x "$NEW_ENGINE" ]]; then
  echo "Error: new engine not executable: $NEW_ENGINE"
  exit 1
fi

if [[ ! -x "$OLD_ENGINE" ]]; then
  echo "Error: old engine not executable: $OLD_ENGINE"
  exit 1
fi

if [[ ! -f "$OPENINGS_FILE" ]]; then
  echo "Error: openings file missing: $OPENINGS_FILE"
  exit 1
fi

PGN_OUT="$OUT_DIR/match.pgn"
LOG_OUT="$OUT_DIR/cutechess.log"
META_OUT="$OUT_DIR/meta.txt"

{
  echo "date: $(date)"
  echo "cutechess: $CUTECHESS"
  echo "new: $NEW_ENGINE"
  echo "old: $OLD_ENGINE"
  echo "games: $GAMES"
  echo "tc: $TC"
  echo "concurrency: $CONCURRENCY"
  echo "openings: $OPENINGS_FILE"
  shasum "$NEW_ENGINE" "$OLD_ENGINE" || true
} > "$META_OUT"

echo "Running self-play benchmark..."
echo "Output dir: $OUT_DIR"

set +e
"$CUTECHESS" \
  -engine name=chessmatrix_new cmd="$NEW_ENGINE" proto=uci \
  -engine name=chessmatrix_old cmd="$OLD_ENGINE" proto=uci \
  -each tc="$TC" \
  -games "$GAMES" \
  -rounds 1 \
  -repeat \
  -concurrency "$CONCURRENCY" \
  -openings file="$OPENINGS_FILE" format=epd order=random \
  -pgnout "$PGN_OUT" \
  -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 \
  2>&1 | tee "$LOG_OUT"
EXIT_CODE=${PIPESTATUS[0]}
set -e

echo
if [[ $EXIT_CODE -eq 0 ]]; then
  echo "Benchmark completed successfully."
else
  echo "Benchmark exited with code: $EXIT_CODE"
fi

echo "Results: $OUT_DIR"
echo "- Log: $LOG_OUT"
echo "- PGN: $PGN_OUT"
echo "- Meta: $META_OUT"

exit $EXIT_CODE
