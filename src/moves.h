#pragma once
#include <stdlib.h>
#include "commons.h"

void CreateMoves(Game* game);
void CreateCaptureMoves(Game* game);
void RemoveInvalidMoves(Game* game);
int ValidMoves(Move* moves);
Undos DoMove(Move move, Game* mainGame);
void UndoMove(Game* game, Move move, Undos undos);
void DoNullMove(Game* game);
void UndoNullMove(GameState prevGameState, Game* game, U64 prevHash);

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
Move CreateNextMove(Game* game, int* pPiece, int* pPattern, int* pRay, int* pPawnCaptPat, int* pCastleCount);
Move CreateNextCaptureMove(Game* game, int* pPiece, int* pPattern, int* pRay, int* pPawnCaptPat);
