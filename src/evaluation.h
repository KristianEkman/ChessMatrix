#include "basic_structs.h"

#pragma once
//[type][side][square]
short PositionValueMatrix[7][2][64];

//[side][short/long]
short CastlingPoints[2];

//[middle or end][side][square]
short KingPositionValueMatrix[2][2][64];

short GetEval(Game* game);