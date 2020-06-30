#pragma once
#include <stdbool.h>

#define MAX_SCORE 8190 //14 bits
#define MIN_SCORE -8190

typedef unsigned long long U64;
typedef unsigned int uint;

typedef enum {
	NOPIECE,
	BISHOP,
	ROOK,
	QUEEN,
	PAWN,
	KNIGHT,
	KING, // 6	
} PieceType;

typedef enum {
	WHITE = 8, //01000 >> 4 ger 0
	BLACK = 16 //10000 >> 4 ger 1
} Side;

typedef enum {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
} Square;

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
	EnPassantCapture = 10,
	NotAMove = 11,
	QueenMove = 12
} MoveInfo;

typedef struct {
	char From;
	char To;
} MoveCoordinates;

typedef struct {
	char From;
	char To;
	char MoveInfo;
	short Score;
	// Index of the piece moved in Pieces List.
	char PieceIdx;
} Move;

typedef enum {
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

typedef struct {
	PieceType Type;
	bool Off;
	unsigned char SquareIndex;
	unsigned char MoveCount;
	//unsigned char Mobility; // this performed bad, but nice idea
} Piece;


typedef struct {
	int CaptIndex;
	GameState PrevGameState;
	short PrevPositionScore;
	U64 PrevHash;
} Undos;

typedef struct {
	Move Move;
	Undos Undos;
	bool Invalid;
} PlayerMove;


typedef struct {
	Side Side;
	int Side01;
	int MovesBufferLength;
	Move MovesBuffer[100];
	int KingSquares[2];
	GameState State;
	short Material[2];
	PieceType Squares[64];
	short PositionScore;
	U64 Hash;
	U64 PositionHistory[2000];
	int PositionHistoryLength;
	Piece Pieces[2][16];
	Move KillerMoves[31][2];
} Game;

typedef struct {
	int ThreadID;
	int MoveCount;
	int Depth;
} ThreadParams;

typedef struct {
	Move Moves[100];
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

Game g_mainGame;
