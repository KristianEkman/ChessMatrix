#pragma once
#include <stdlib.h>
#include "commons.h"

void CreateMoves(Game* game, int depth);
void CreateCaptureMoves(Game* game);
void RemoveInvalidMoves(Game* game);
int ValidMoves(Move* moves);
int MakeMove(Move move, Game* mainGame);
void UnMakeMove(Move move, int captIndex, GameState prevGameState, short prevPositionScore, Game* game, U64 prevHash);
void MakeNullMove(Game* game);
void UnMakeNullMove(GameState prevGameState, Game* game, U64 prevHash);

void AssertGame(Game* game);

PlayerMove MakePlayerMove(char* sMove);
void UnMakePlayerMove(PlayerMove move);
PlayerMove MakePlayerMoveOnThread(Game* game, char* sMove);
void UnMakePlayerMoveOnThread(Game* game, PlayerMove playerMove);
void MoveToString(Move move, char * sMove);
Move ParseMove(char* sMove, MoveInfo info);
bool SquareAttacked(int square, Side attackedBy, Game* game);
void SortMoves(Move* moves, int moveCount, Side side);

