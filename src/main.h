#pragma once
#include "basic_structs.h"

#define SEARCH_THREADS 7
#define ASPIRATION_WINDOW_SIZE 25

void PrintGame();
void ReadFen(char * fen);
void WriteFen(char * fen);
int ValidMoves(Move * moves);
PlayerMove MakePlayerMove(char * sMove);
void UnMakePlayerMove(PlayerMove move);

Move parseMove(char * sMove, MoveInfo info);
Game mainGame;
short TotalMaterial(Game * game);
int SearchedLeafs;
Move BestMoveAtDepthDeepening(int maxDepth);
int GetScore(Game * game);
short GetBestScore(Game * game, int depth);
void CreateMoves(Game * game, int depth);
void MakeMove(Move move, Game * mainGame);
void UnMakeMove(Move move,PieceType capture,GameState prevGameState, int prevPositionScore, Game * game, unsigned long long prevHash);
const int MOVESIZE;