#pragma once
#include "common.h"

#define MAX_SCORE 8190 //14 bits
#define MIN_SCORE -8190

typedef enum  {
	NOPIECE,
	BISHOP,
	ROOK,
	QUEEN,
	PAWN,
	KNIGHT,
	KING, // 6	
} PieceType;

// PieceType is combined with Side color define a piece.
typedef enum Side {
	WHITE = 8, //01000 >> 4 ger 0
	BLACK = 16 //10000 >> 4 ger 1
} Side;

typedef enum {
	false,
	true
} bool;

typedef enum {
	PlainMove = 0,
	PromotionQueen = 1,
	PromotionRook = 2,
	PromotionBishop = 3,
	PromotionKnight = 4,
	KingMove = 5,
	RookMove = 6,
	CastleShort = 7,
	CastleLong = 8,
	EnPassant = 9,
	EnPassantCapture = 10, //First 4 bits
	NotAMove = 100
	//Bishop or Knight
	//Queen
	//
}MoveInfo;

typedef struct Move {
	char From; //0-63, 6bit, mask 3F
	char To; //0-63, 6bit, mask FC0, 4032
	char MoveInfo; //4 bits, 13-16
	short ScoreAtDepth; //14 bits
	char PieceIdx;
} Move;

typedef enum  {
	EnPassantfile, //mask 1111 (15)
	WhiteCanCastleShort = 16,
	WhiteCanCastleLong = 32,
	BlackCanCastleShort = 64,
	BlackCanCastleLong = 128
} GameState;

typedef struct {
	int Captures;
	int Enpassants;
	int Castles;
	int Promotions;
	int Checks;
	int CheckMates;
} PerftResult;


typedef struct Piece {
	PieceType Type;
	bool Off;
	int SquareIndex;
} Piece;

typedef struct {
	Move Move;
	int CaptureIndex;
	GameState PreviousGameState;
	bool Invalid;
	U64 PreviousHash;
} PlayerMove;


typedef struct {
	Side Side;
	int MovesBufferLength;
	Move MovesBuffer[100];
	int KingSquares[2];
	GameState State;
	short Material[2];
	PieceType Squares[64];
	U64 Hash;
	U64 PositionHistory[2000];
	int PositionHistoryLength;
	int ThreadIndex;
	Piece Pieces[2][16];
} Game;

typedef struct {
	int threadID;
	int moveCount;
	int depth;
} ThreadParams;

typedef struct {
	Move moves[100];
	int Length;
} GlobalRootMoves;

typedef struct {
	int MaxDepth;
	int MoveTime;
	int WhiteTimeLeft;
	int BlackTimeLeft;
	int BlackIncrement;
	int WhiteIncrement;
	int MovesTogo;
	bool TimeControl;
	Move BestMove;
} TopSearchParams;

TopSearchParams g_topSearchParams;
