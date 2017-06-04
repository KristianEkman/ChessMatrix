#pragma once

enum PieceType {
	NoPiece,
	Pawn,
	Bishop,
	Knight,
	Rook,
	Queen,
	King
};

enum Color {
	NoColor,
	Black,
	White
};

typedef struct  {
	enum PieceType Type;
	enum Color Color;
} Piece;

typedef struct {
	Piece Piece;
} Square;

typedef struct {
	Square Squares[64];
} Game;

typedef struct {
	int From;
	int To;
} Move;