#pragma once
#include "basic_structs.h"

int Perft(int depth);
void PrintGame();
void ReadFen(char * fen);
void WriteFen(char * fen);
int ValidMoves(Move * moves);
int MakePlayerMove(char * sMove);
PerftResult _perftResult;
Move parseMove(char * sMove, MoveInfo info);
short GameMaterial;