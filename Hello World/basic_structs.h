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
	char * RookRays[5];
	char * BishopRays[5];
	char * QueenRays[9];
	char * EmptyRayPattern[1];
	char * KnightsPattern;
	char * KingsPattern;
	char * WhitePawnCaptures;
	char * WhitePawnPattern;
	char * BlackPawnCaptures;
	char * BlackPawnPattern;
	char Index;
} Square;

char * bishopRayCount = { 4 };
char * rookRayCount = { 4 };
char * queenRayCount = { 8 };
char * noRaysCount = { 0 };

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