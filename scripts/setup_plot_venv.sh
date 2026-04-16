#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${1:-$ROOT_DIR/.venv-plot}"

python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"
python3 -m pip install -r "$ROOT_DIR/scripts/requirements-plot.txt"

echo
echo "Plotting environment ready: $VENV_DIR"
echo "Activate it with: source $VENV_DIR/bin/activate"