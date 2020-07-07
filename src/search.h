#pragma once
#include <stdlib.h>
#include <Windows.h>
#include "commons.h"

uint g_SearchedNodes;
bool g_Stopped;
TopSearchParams g_topSearchParams;
void SetSearchDefaults();
MoveCoordinates Search(bool async);
void InitLmr();
