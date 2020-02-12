#pragma once
#include "commons.h"

void PrintGame(Game * game);
void ReadFen(char * fen);
void WriteFen(char * fen);

void SwitchSignOfWhitePositionValue();
void InitGame();
void InitScores();
void InitHash();

void EnterUciMode();
int EnterInteractiveMode();
void AdjustPositionImportance();
