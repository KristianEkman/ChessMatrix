#include "basic_structs.h"

#pragma once
//[type][side][square]
char PositionValueMatrix[7][2][64];

//[side][short/long]
char CastlingPoints[2][2];

//[middle or end][side][square]
char KingPositionValueMatrix[2][2][64];

short GetEval(Game* game);