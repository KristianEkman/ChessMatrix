#pragma once

typedef enum  {
	NoPiece,
	Bishop,
	Rook,
	Queen,
	Pawn,
	Knight,
	King,
	White = 8, //01000 >> 4 ger 0
	Black = 16 //10000 >> 4 ger 1
} PieceType;


typedef struct {
	PieceType Squares[64];
} Game;

typedef enum {
	false,
	true
} bool;

typedef enum {
	PlainMove = 0,
	Promotion = 1,
	KingMove = 2,
	RookMove = 4,
	CastleShort = 8,
	CastleLong = 16
}MoveInfo;

typedef struct {
	char From; //0-63, 6bit, mask 3F
	char To; //0-63, 6bit, mask FC0, 4032
	MoveInfo MoveInfo; //5bit, mask 
	//could shrink to 17bits, move could easily fit into 4byte int
} Move;

typedef enum  {
	EnPassantfile, //mask 1111
	WhiteCanCastleShort = 16,
	WhiteCanCastleLong = 32,
	BlackCanCastleShort = 64,
	BlackCanCastleLong = 128,
	//sum 8 bits
} GameState;