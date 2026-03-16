#pragma once
#include <stdlib.h>
#include "commons.h"

typedef enum {
	FastMoveIllegal = 0,
	FastMoveLegal = 1,
	FastMoveNeedsFullCheck = 2
} FastMoveLegality;

typedef struct {
	int KingSquare;
	uchar CheckCount;
	U64 Checkers;
	U64 EvasionMask;
	U64 Pinned;
	U64 PinLineMasks[64];
} LegalMoveContext;

void CreateMoves(Game* game);
void CreateCaptureMoves(Game* game);
void RemoveInvalidMoves(Game* game);
int ValidMoves(Move* moves);
int ValidMovesOnThread(Game* game, Move* moves);
Undos DoMove(Move move, Game* mainGame);
void UndoMove(Game* game, Move move, Undos undos);
bool DoNullMove(Game* game);
void UndoNullMove(GameState prevGameState, Game* game, U64 prevHash, bool positionHistoryPushed);
void BuildLegalMoveContext(Game *game, LegalMoveContext *ctx);
FastMoveLegality ClassifyMoveLegality(Move move, Game *game, const LegalMoveContext *ctx);

void AssertGame(Game* game);

PlayerMove MakePlayerMove(char* sMove);
void UnMakePlayerMove(PlayerMove move);
PlayerMove MakePlayerMoveOnThread(Game* game, char* sMove);
void UnMakePlayerMoveOnThread(Game* game, PlayerMove playerMove);
void MoveToString(Move move, char * sMove);
Move ParseMove(char* sMove, MoveInfo info);
bool SquareAttacked(int square, Side attackedBy, Game* game);
void SortMoves(Move* moves, int moveCount, Side side);
void CoordinatesToString(MoveCoordinates move, char* sMove);

