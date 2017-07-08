#pragma once
#include "basic_structs.h"

int Perft(int depth);
void PrintGame();
void ReadFen(char * fen);
void WriteFen(char * fen);
int ValidMoves(Move * moves);
PlayerMove MakePlayerMove(char * sMove);
void UnMakePlayerMove(PlayerMove move);

Move parseMove(char * sMove, MoveInfo info);
Game game;
short TotalMaterial();
int SearchedLeafs;
Move BestMoveAtDepthDeepening(int maxDepth);
int GetScore();
