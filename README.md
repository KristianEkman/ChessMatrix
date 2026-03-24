# ChessMatrix

This is a chess engine written in C.
Strength is estimated at between 2000-2100 ELO.
It is currently under development.

Features:

- 64-square board representation
- Piece lists
- Piece move templates
- Zobrist-hashed transposition table with configurable size
- Iterative deepening alpha-beta search
- Null-move pruning
- Quiescence search
- Late move reductions
- Move ordering heuristics, including countermoves
- Opening book in ABK format
- UCI protocol

## Build

Windows 10:

- Open the solution in Visual Studio 2019 and build the project.

Linux:

- Install a C11 compiler and make.
- Run make in the repository root.

macOS:

- Install Xcode Command Line Tools or another C11-compatible compiler.
- Run make in the repository root.
