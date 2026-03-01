#pragma once
#include "commons.h"

void StartPosition();
void InitPieceList();
void PutFreePieceAt(int square, PieceType pieceType, int side01);
void InitPiece(int file, int rank, PieceType type, Side color);
void InitScores();
void InitHash();
void FixPieceChain(Game* game);
