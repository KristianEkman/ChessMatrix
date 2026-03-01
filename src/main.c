#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "commons.h"
#include "platform.h"
#include "fen.h"
#include "position.h"
#include "patterns.h"
#include "main.h"
#include "utils.h"
#include "tests.h"
#include "evaluation.h"
#include "sort.h"
#include "hashTable.h"
#include "timeControl.h"
#include "moves.h"
#include "search.h"
#include "book.h"
#include "version.h"
#include "uci.h"

Game g_mainGame = {0};

void ComputerMove()
{
	g_topSearchParams.MoveTime = 5000;
	MoveCoordinates move = Search(false);
	char sMove[6];
	CoordinatesToString(move, sMove);
	MakePlayerMove(sMove);
}

void ManualMove()
{
	printf("\nYour move (format e2e4 or e7e8q): ");
	char sMove[8];
	if (fgets(sMove, 8, stdin) == NULL)
		return;
	fseek(stdin, 0, SEEK_END);
	PlayerMove pm = MakePlayerMove(sMove);
	if (pm.Invalid)
		Stdout_wl("Invalid move");
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	const char *disableSignalHandlers = getenv("CM_DISABLE_SIGNAL_HANDLERS");
	if (disableSignalHandlers == NULL || disableSignalHandlers[0] == '\0' || disableSignalHandlers[0] == '0')
	{
		InstallUnhandledErrorHandlers();
	}
	SwitchSignOfWhitePositionValue();
	SetSearchDefaults();
	ResetDepthTimes();
	GenerateZobritsKeys();
	AllocateHashTable(1024);
	ClearHashTable();
	InitLmr();
	CalculatePatterns();
	StartPosition();
	OwnBook = false;
#ifndef _DEBUG // loadbook is to slow in debug mode.
			   // LoadBook("openings.abk");
#endif		   // DEBUG

	printf("info Built date %s\n", BuildDate);
	printf("info Branch %s\n", GitBranch);
	printf("info Commit %s\n", GitCommit);
	printf("Initialized\n");
	EnterUciMode();
	return 0;
}

void PrintOptions()
{
	printf("\nm: make move\n");
	printf("c: computer move\n");
	printf("t: run tests\n");
	printf("g: print game\n");
	printf("e: eval\n");
	printf("h: help\n");
	printf("q: quit\n");
}

int EnterInteractiveMode()
{
	char buffer[20];
	char in = ' ';
	PrintGame(&g_mainGame);
	PrintOptions();

	while (in != 'q')
	{
		// system("@cls||clear");
		printf("h for help> ");
		if (fgets(buffer, 20, stdin) == NULL)
			break;
		if (buffer[1] == '\n')
			in = buffer[0];
		fseek(stdin, 0, SEEK_END);
		switch (in)
		{
		case 'm':
			ManualMove();
			PrintGame(&g_mainGame);
			break;
		case 'c':
			ComputerMove();
			PrintGame(&g_mainGame);
			break;
		case 'g':
			PrintGame(&g_mainGame);
			break;
		case 't':
			runAllTests();
			break;
		case 'e':
			printf("Eval: %d\n", GetEval(&g_mainGame));
			break;
		case 'h':
			PrintOptions();
			break;
		case 'q':
			break;
		default:
			Stdout_wl("Not an option");
			break;
		}
	}

	return 0;
}

void StartPosition()
{
	InitPieceList();

	for (int i = 0; i < 64; i++)
		g_mainGame.Squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	g_mainGame.KingSquares[0] = 4;
	InitPiece(5, 0, BISHOP, WHITE);
	InitPiece(6, 0, KNIGHT, WHITE);
	InitPiece(7, 0, ROOK, WHITE);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 1, PAWN, WHITE);

	InitPiece(0, 7, ROOK, BLACK);
	InitPiece(1, 7, KNIGHT, BLACK);
	InitPiece(2, 7, BISHOP, BLACK);
	InitPiece(3, 7, QUEEN, BLACK);
	InitPiece(4, 7, KING, BLACK);
	g_mainGame.KingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	g_mainGame.Side = WHITE;
	g_mainGame.Side01 = 0;

	g_mainGame.State = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
	g_mainGame.Material[0] = 0;
	g_mainGame.Material[1] = 0;
	g_mainGame.PositionHistoryLength = 0;

	InitHash();
	InitScores();
	g_mainGame.FiftyMoveRuleCount = 0;
}

void PrintGame(Game *game)
{
	char top[] = "+---+---+---+---+---+---+---+---+";
	char rowLine[] = "+---+---+---+---+---+---+---+---+";
	char lastLine[] = "+---+---+---+---+---+---+---+---+";
	char vBorder = '|';

	printf("   %s\n", top);

	for (int r = 8 - 1; r >= 0; r--)
	{
		printf(" %d", r + 1);
		printf(" %c", vBorder);
		for (int f = 0; f < 8; f++)
		{
			printf(" ");
			PieceType piece = game->Squares[r * 8 + f];
			Side side = piece & 24;
			char c[] = {PieceChar(piece), 0};
			if (side == BLACK)
				ColorPrint(c, lightblue, black);
			else
				ColorPrint(c, yellow, black);

			if (f < 7)
				printf(" |");
			else
				printf(" %c\n", vBorder);
		}
		if (r > 0)
			printf("   %s\n", rowLine);
	}
	printf("   %s\n", lastLine);
	printf("     a   b   c   d   e   f   g   h  \n");
	printf("Hash: %llx\n", game->Hash);
	char fen[100];
	WriteFen(fen);
	printf("FEN: %s\n", fen);
}




