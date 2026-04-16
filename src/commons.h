#pragma once

#if defined(_DEBUG)
#include <assert.h>
#define CM_DEBUG_ASSERT(expr) assert(expr)
#else
#define CM_DEBUG_ASSERT(expr) ((void)0)
#endif

#define false 0
#define true 1

#define MAX_SCORE 8190 //14 bits
#define MIN_SCORE -8190
#define MAX_DEPTH 63 // hash stores depth in 6 bits
#define MAX_MOVES 256
#define MAX_POSITION_HISTORY 4096

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

//white, black   nopiece, bishop, rook, queen, pawn, knight, promotions (minus pawn)
#define MATERIAL_B 325
#define MATERIAL_R 550
#define MATERIAL_Q 1000
#define MATERIAL_P 100
#define MATERIAL_N 320
#define MATERIAL_N_N 640

typedef char bool;
typedef unsigned long long U64;
typedef unsigned int uint;
typedef unsigned char uchar;

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
} Square; // not used

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
	QueenMove = 12,
	// move to 7th or 2nd rank
	SoonPromoting = 13
} MoveInfo;

typedef struct {
	uchar From;
	uchar To;
} MoveCoordinates;

typedef struct {
	uchar From; // Using uchar type instead of Square enum keeps the struct compact while avoiding signed char index warnings.
	uchar To;
	uchar MoveInfo;
	short Score;
	// Index of the piece moved in Pieces List.
	uchar PieceIdx;
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

typedef struct
{
	U64 WhitePawns;
	U64 BlackPawns;
	U64 AllPawns;
} PawnBitboards;

typedef struct
{
	U64 WhiteKnights;
	U64 BlackKnights;
	U64 AllKnights;
} KnightBitboards;

typedef struct
{
	U64 WhiteBishops;
	U64 BlackBishops;
	U64 AllBishops;
} BishopBitboards;

typedef struct
{
	U64 WhiteRooks;
	U64 BlackRooks;
	U64 AllRooks;
} RookBitboards;

typedef struct
{
	U64 WhiteQueens;
	U64 BlackQueens;
	U64 AllQueens;
} QueenBitboards;

typedef struct
{
	U64 WhiteKing;
	U64 BlackKing;
	U64 AllKings;
} KingBitboards;

typedef struct
{
	PawnBitboards Pawns;
	KnightBitboards Knights;
	BishopBitboards Bishops;
	RookBitboards Rooks;
	QueenBitboards Queens;
	KingBitboards Kings;
	U64 WhitePieces;
	U64 BlackPieces;
	U64 Occupied;
	U64 Matrix[7][2];
} AllPieceBitboards;


typedef struct Piece {
	PieceType Type;
	bool Off;
	uchar Index;
	uchar SquareIndex;
	uchar MoveCount;
	// Next Piece in the Piece Link List
	struct Piece* Next;
	// Previous Piece in the Piece Link List
	struct Piece * Prev;
	//unsigned char Mobility; // this performed bad, but nice idea
} Piece;


typedef struct {
	int CaptIndex;
	GameState PrevGameState;
	U64 PrevHash;
	uchar FiftyMoveRuleCount;
	bool PositionHistoryPushed;
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
	Move MovesBuffer[MAX_MOVES];
	int KingSquares[2];
	GameState State;
	short Material[2];
	PieceType Squares[64];
	AllPieceBitboards Bitboards;
	U64 Hash;
	U64 PositionHistory[MAX_POSITION_HISTORY];
	int PositionHistoryLength;
	Piece Pieces[2][16];
	//MoveCoordinates KillerMoves[500][2];
	uchar FiftyMoveRuleCount;
} Game;

typedef struct {
	int ThreadID;
	int MoveCount;
	int Depth;
} ThreadParams;

typedef struct {
	Move Moves[MAX_MOVES];
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
	bool EmitInfoLines;
	bool HasSearchScore;
	short SearchScore;
} TopSearchParams;

extern Game g_mainGame;
