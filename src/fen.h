#pragma once
#include "commons.h"

char PieceChar(PieceType pieceType);
PieceType parsePieceType(char c);
Side parseSide(char c);
void ReadFen(const char *fen);
void WriteFen(char *fenBuffer);
