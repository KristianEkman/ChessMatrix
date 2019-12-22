#include "basic_structs.h"

#pragma once
//[type][side][square]
char PositionValueMatrix[7][2][64];

//[middle or end][side][square]
char KingPositionValueMatrix[2][2][64];

short GetEval(Game* game);