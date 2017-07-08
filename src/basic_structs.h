#pragma once

typedef enum  {
	NOPIECE,
	BISHOP,
	ROOK,
	QUEEN,
	PAWN,
	KNIGHT,
	KING,
	WHITE = 8, //01000 >> 4 ger 0
	BLACK = 16 //10000 >> 4 ger 1
} PieceType;

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
	EnPassantCapture = 10,
	//Bishop or Knight
	//Queen
	//
}MoveInfo;

typedef struct {
	char From; //0-63, 6bit, mask 3F
	char To; //0-63, 6bit, mask FC0, 4032
	char MoveInfo; //4 bits, 13-16
	short ScoreAtDepth;
	//could shrink
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

typedef struct {
	Move Move;
	PieceType Capture;
	GameState PreviousGameState;
	bool Invalid;
	int PreviousPositionScore;
	//unsigned long long PreviousHash;
} PlayerMove;

typedef struct {
	char Side;
	int MovesBufferLength;
	Move MovesBuffer[100];
	int KingSquares[2];
	GameState State;
	short Material[2];
	PieceType Squares[64];
	PerftResult PerftResult;
	int _perftCount;
	int PositionScore;
	//unsigned long long Hash;

} Game;


typedef enum  {
	HT_EmptyEntry = 0,
	HT_NegScore = 1, //bit 1
	HT_Score = 0x3FFE, //max 8191, 13 bits, no 2 tom 14
	HT_SearchDepth = 0x1F, //max 31, 5 bit, no 15 tom 19
	HT_SecondKey = 0xFFF //bit 20-32, 13 bits could be used as second key
}HashTableEntry;