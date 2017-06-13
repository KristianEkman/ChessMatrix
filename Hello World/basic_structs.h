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
	int * RookRays[5];
	int * BishopRays[5];
	int * QueenRays[9];
	int * EmptyRayPattern[1];
	int * KnightsPattern;
	int * KingsPattern;
	int * WhitePawnCaptures;
	int * WhitePawnPattern;
	int * BlackPawnCaptures;
	int * BlackPawnPattern;
	int Index;
} Square;

int * bishopRayCount = { 4 };
int * rookRayCount = { 4 };
int * queenRayCount = { 8 };
int * noRaysCount = { 0 };

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
	bool Promotion;
} Move;