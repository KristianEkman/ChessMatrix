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
	Black,
	White
};

typedef struct  {
	enum PieceType Type;
	enum Color Color;
} Piece;

typedef struct {
	Piece Piece;
	int * RookRays[4];
	int * BishopRays[4];
	int * QueenRays[8];
	int * KnightsPattern;
	int * KingsPattern;
	int * WhitePawnCaptures;
	int * WhitePawnPattern;
	int * BlackPawnCaptures;
	int * BlackPawnPattern;
} Square;

typedef struct {
	Square Squares[64];
} Game;

typedef enum {
	false,
	true
} bool;

typedef struct {
	Square * From;
	Square * To;
	Piece Capture;
	bool Legal;
} Move;