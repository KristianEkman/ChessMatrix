#pragma once

typedef enum  {
	NoPiece,
	Bishop,
	Rook,
	Queen,
	Pawn,
	Knight,
	King,
	White = 8,
	Black = 16
} PieceType;

typedef struct {
	PieceType Squares[64];
} Game;

typedef enum {
	false,
	true
} bool;

typedef struct {
	char From;
	char To;
	PieceType Capture;
	bool Legal;
	bool Promotion;
} Move;