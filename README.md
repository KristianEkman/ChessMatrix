# ChessMatrix

This is a chess engine written in C.
Strength is estimated at between 2000-2100 ELO.
It is currently under development.

Features:

- 64 square board representation
- Piece list
- Piece move templates
- Zobrist Hash Transposition table at configurable size.
- Alpha beta pruning
- Opening book in abk format.
- Null move pruning
- Quiescence search
- Late move reductions
- UCI Protocol

Complie solution on Windows 10 using Visual Studio 2019.

Run all tests with `make test`.
Run one exact test by name with `make test PerftTestStart`.
The variable form `make test testName=PerftTestStart` also works.
You can also call the binary directly with `./chessmatrix test` or `./chessmatrix test PerftTestStart`.
Run the built-in benchmark suite with `make bench` or `./chessmatrix bench`.
Override the benchmark depth with `make bench depth=9` or `./chessmatrix bench 9`.
Benchmark with ANN eval enabled by loading a phase-bundled ANN file directly: `./chessmatrix bench 7 --ann-eval ann_training_weights.txt --ann-blend 10 --ann-max-correction 30 --ann-min-phase 12 --ann-max-base-eval 150`.

ANN self-play training mode is available through `./chessmatrix train-ann`.
This links against the sibling `../ANN` repository and builds `../ANN/build/libANN_lib.a` through the ChessMatrix `Makefile` when needed.
Example: `./chessmatrix train-ann --games 10 --movetime 50 --opening-plies 4 --save ann_training_weights.txt`.
Resume from an existing phase-bundled weights file with `./chessmatrix train-ann --load ann_training_weights.txt --save ann_training_weights.txt`.
The saved file now contains one ANN per game phase, and runtime ANN eval selects the exact phase network for each position.
The trainer now uses 64 board-only inputs from the side-to-move perspective, so older 65-input weights files from the previous side-feature layout will not load into the new shape. Older 64-input single-network files still load and are replicated across phases on import.
Use `--learn-rate X` to set the base SGD step size; the default is `0.05`.
Adaptive learning-rate decay now watches validation average squared error and is enabled by default. Use `--fixed-learn-rate` to keep a constant rate, `--adaptive-learn-rate` to turn the scheduler back on, and tune the scheduler with `--learn-rate-min X`, `--learn-rate-decay X`, `--learn-rate-patience N`, and `--learn-rate-epsilon X`.
Use `--progress-interval N` to print rolling learning stats such as average absolute error and average squared error every N trained roots.
Use `--updates-per-position N` to apply multiple SGD steps to each searched root before moving on; the default is 4.
Use `--validation-depth N` to build the validation targets once at a fixed search depth; the default is 6.
Use `--progress-csv training_progress.csv` to write those progress snapshots as CSV rows for plotting, including the current learning rate at each checkpoint.
The trainer now builds a larger fixed validation FEN set once at startup, clears hash state while generating those targets, and then reuses the cached targets whenever it emits progress. That makes validation curves much less sensitive to time-control noise than the earlier live re-searching approach.
Per-root trainer lines print normalized targets and outputs plus a centipawn equivalent. The current trainer uses a tanh mapping with `ANN_TRAINING_SCORE_SCALE` set to 400cp, so ordinary opening scores produce a much stronger learning signal than the earlier wide linear scaling.
Runtime ANN evaluation is integrated as an optional blended correction on top of the classical evaluation. It is disabled by default, uses a small hash-based cache to avoid recomputing repeated positions, selects the ANN that matches the current 0..24 game phase, and exposes `AnnEval`, `AnnEvalFile`, `AnnEvalBlend`, `AnnEvalMaxCorrection`, `AnnEvalMinPhase`, and `AnnEvalMaxBaseEval` through UCI for A/B testing.
Plot a saved CSV with `python3 scripts/plot_ann_progress.py training_progress.csv`.
Use `python3 scripts/plot_ann_progress.py training_progress.csv --watch` to keep redrawing while training is still appending to the CSV.
The plotter now shows train and validation error plus the learning-rate schedule over time.
The plotter uses `matplotlib`; on macOS/Homebrew Python install it in a small venv:

```bash
bash scripts/setup_plot_venv.sh
source .venv-plot/bin/activate
python3 scripts/plot_ann_progress.py training_progress.csv --watch
```

If you see `-r option requires 1 argument`, the `pip install -r ...` command was run without the requirements path. Using `bash scripts/setup_plot_venv.sh` avoids that copy/paste issue.
