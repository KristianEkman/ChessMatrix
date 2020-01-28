#pragma once
#include "commons.h"


void PrintGame(Game * game);
void ReadFen(char * fen);
void MoveToString(Move move, char sMove[5]);
void WriteFen(char * fen);
int ValidMoves(Move * moves);

Move ParseMove(char * sMove, MoveInfo info);
short TotalMaterial(Game * game);
DWORD WINAPI BestMoveDeepening(void * params);

void SwitchSignOfWhitePositionValue();
void InitGame();
void InitScores();

void EnterUciMode();
int EnterInteractiveMode();
void InitScores();
void InitHash();
DWORD WINAPI TimeLimitWatch(void* pms);
void AdjustPositionImportance();
void DefaultSearch();
