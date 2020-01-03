#pragma once
#include "basic_structs.h"
#define SEARCH_THREADS 7

void PrintGame();
void ReadFen(char * fen);
void MoveToString(Move move, char sMove[5]);
void WriteFen(char * fen);
int ValidMoves(Move * moves);
PlayerMove MakePlayerMove(char * sMove);
void UnMakePlayerMove(PlayerMove move);

Move parseMove(char * sMove, MoveInfo info);
Game mainGame;
short TotalMaterial(Game * game);
int SearchedLeafs;
DWORD WINAPI BestMoveDeepening(void * params);
short GetScore(Game * game);
void CreateMoves(Game * game, int depth);
void MakeMove(Move move, Game * mainGame);
void UnMakeMove(Move move,PieceType capture,GameState prevGameState, short prevPositionScore, Game * game, unsigned long long prevHash);

void SwitchSignOfWhitePositionValue();
void InitGame();
void InitScores();

void EnterUciMode();
int EnterInteractiveMode();
void InitScores();
void InitHash();
Move Search(bool async);
DWORD WINAPI TimeLimitWatch(int* pms);
void AdjustPositionImportance();
void DefaultSearch();
