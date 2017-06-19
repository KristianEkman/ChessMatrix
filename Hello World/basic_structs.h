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
//
//typedef struct {
//	PieceType Squares[64];
//} Game;

typedef enum {
	false,
	true
} bool;

typedef enum {
	PlainMove = 0,
	Promotion = 1,
	KingMove = 2,
	CastleShort = 4,
	CastleLong = 8
}MoveInfo;

typedef struct {
	char From;
	char To;
	PieceType Capture;
	bool Legal;
	MoveInfo MoveInfo;
} Move;
