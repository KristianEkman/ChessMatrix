#pragma once
#include <stdlib.h>
#include "platform.h"
#include "commons.h"

extern uint g_SearchedNodes;
extern bool g_Stopped;
extern TopSearchParams g_topSearchParams;
void SetSearchDefaults();
MoveCoordinates Search(bool async);
void StopSearch();
void InitLmr();
void FixPieceChain(Game* game);
