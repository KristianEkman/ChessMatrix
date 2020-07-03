#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <Windows.h>
#include <stdio.h>

#include "commons.h"
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


void ComputerMove() {
	g_topSearchParams.MoveTime = 5000;
	MoveCoordinates move = Search(false);
	char sMove[5];
	CoordinatesToString(move, sMove);
	MakePlayerMove(sMove);
}

void ManualMove() {
	printf("\nYour move (format e2e4): ");
	char sMove[5];
	fgets(sMove, 5, stdin);
	fseek(stdin, 0, SEEK_END);
	PlayerMove pm = MakePlayerMove(sMove);
	if (pm.Invalid)
		Stdout_wl("Ivalid move");
}

void InitPieceList() {
	char side[2] = { WHITE, BLACK };
	for (int s = 0; s < 2; s++)
	{
		for (int p = 0; p < 8; p++)
			g_mainGame.Pieces[s][p].Type = PAWN | side[s];

		g_mainGame.Pieces[s][8].Type = ROOK | side[s];
		g_mainGame.Pieces[s][9].Type = ROOK | side[s];

		g_mainGame.Pieces[s][10].Type = KNIGHT | side[s];
		g_mainGame.Pieces[s][11].Type = KNIGHT | side[s];

		g_mainGame.Pieces[s][12].Type = BISHOP | side[s];
		g_mainGame.Pieces[s][13].Type = BISHOP | side[s];

		g_mainGame.Pieces[s][14].Type = QUEEN | side[s];
		g_mainGame.Pieces[s][15].Type = KING | side[s];

		for (int p = 0; p < 16; p++)
		{
			g_mainGame.Pieces[s][p].Off = true;
			g_mainGame.Pieces[s][p].MoveCount = 0;
		}
	}
}

int main(int argc, char* argv[]) {
	SwitchSignOfWhitePositionValue();
	SetSearchDefaults();
	ResetDepthTimes();
	GenerateZobritsKeys();
	AllocateHashTable(1024);
	ClearHashTable();
	StartPosition();

	OwnBook = true;
#ifndef _DEBUG // loadbook is to slow in debug mode.
	LoadBook("openings.abk");
#endif // DEBUG

	printf("initialized\n");
	EnterUciMode();
	return 0;
}

void EnterUciMode() {
#define UCI_BUF_SIZE 5000
	char buf[UCI_BUF_SIZE];
	fgets(buf, 5000, stdin);
	while (!Streq(buf, "quit\n"))
	{
		if (StartsWith(buf, "ucinewgame")) {
			ClearHashTable();
			ResetDepthTimes();
			Stdout_wl("new game");
		}
		else if (StartsWith(buf, "uci")) {
			Stdout_wl("id name CChess");
			Stdout_wl("id author Kristian Ekman");
			Stdout_wl("option name Hash type spin default 1024 min 1 max 2048");
			Stdout_wl("option name OwnBook type check default false");
			Stdout_wl("uciok");
		}
		else if (StartsWith(buf, "isready")) {
			Stdout_wl("readyok");
		}
		else if (StartsWith(buf, "setoption name Hash value")) {
			//setoption name Hash value 32
			char* token = strtok(buf, " ");
			while (token != NULL) {
				if (Streq(token, "value")) {
					token = strtok(NULL, " ");
					int mb = atoi(token);
					if (mb >= 1 && mb <= 2048) { // actual limit is 32760mb
						AllocateHashTable(mb);
						Stdout_wl("new hash size set");
					}
					else {
						Stdout_wl("invalid Hash size value (1 - 2048)");
					}
					break;
				}
				token = strtok(NULL, " ");
			}
		}
		else if (StartsWith(buf, "setoption name OwnBook value")) {
			//setoption name OwnBook value false
			char* token = strtok(buf, " ");
			while (token != NULL) {
				if (Streq(token, "value")) {
					token = strtok(NULL, " ");
					if (Streq("true\n", token)) {
						OwnBook = true;
						Stdout_wl("Opening book switched on");
					}
					else if (Streq("false\n", token)) {
						OwnBook = false;
						Stdout_wl("Opening book switched off");
					}					
					break;
				}
				token = strtok(NULL, " ");
			}
		}
		else if (StartsWith(buf, "position ")) {
			//postion fen | moves
			int movesPos = IndexOf(buf, " moves");
			if (Contains(buf, " fen "))
			{
				char* pFen = strstr(buf, " fen ");
				int fenPos = pFen - buf + 5;

				if (movesPos == -1) { // note sure if "moves" string is mandatory
					movesPos = strlen(buf) - 1;
				}

				int fenLength = movesPos - fenPos; //position fen xxx...yyy moves
				char fen[256];
				memcpy(fen, pFen + 5, fenLength);
				fen[fenLength + 1] = '\0';
				ReadFen(fen);
			}

			if (Contains(buf, "startpos")) {
				StartPosition();
			}

			if (Contains(buf, " moves "))
			{
				char* pMoves = strstr(buf, " moves ");
				char* token = strtok(pMoves, " ");
				while (token != NULL) {
					MakePlayerMove(token);
					token = strtok(NULL, " ");
				}
				printf("string position parsed\n");
				AssertGame(&g_mainGame);
			}
		}
		else if (StartsWith(buf, "go ")) {
			SetSearchDefaults();
			if (Contains(buf, "infinite")) {
				g_topSearchParams.MaxDepth = 40;
				g_topSearchParams.TimeControl = false;
				Search(true);
			}
			else {
				char* token = strtok(buf, " ");
				while (token != NULL) {
					if (Streq(token, "depth")) {
						char* depth = strtok(NULL, " ");
						uint dep;
						if (sscanf(depth, "%d", &dep) == 1)
							g_topSearchParams.MaxDepth = dep - 1;
					}
					else if (Streq(token, "movetime")) {
						char* movetime = strtok(NULL, " ");
						int time = 0;
						if (sscanf(movetime, "%d", &time) == 1)
							g_topSearchParams.MoveTime = time;
					}
					else if (Streq(token, "wtime")) {
						g_topSearchParams.TimeControl = true;
						char* wtime = strtok(NULL, " ");
						sscanf(wtime, "%d", &g_topSearchParams.WhiteTimeLeft);
						//go wtime 900000 btime 900000 winc 0 binc 0
					}
					else if (Streq(token, "btime")) {
						g_topSearchParams.TimeControl = true;
						char* btime = strtok(NULL, " ");
						sscanf(btime, "%d", &g_topSearchParams.BlackTimeLeft);
					}
					else if (Streq(token, "winc")) {
						char* winc = strtok(NULL, " ");
						sscanf(winc, "%d", &g_topSearchParams.WhiteIncrement);
					}
					else if (Streq(token, "binc")) {
						char* binc = strtok(NULL, " ");
						sscanf(binc, "%d", &g_topSearchParams.BlackIncrement);
					}
					else if (Streq(token, "movestogo")) {
						char* binc = strtok(NULL, " ");
						sscanf(binc, "%d", &g_topSearchParams.MovesTogo);
					}
					token = strtok(NULL, " ");
				}

				SetMoveTimeFallBack(g_mainGame.Side);

				fflush(stdout);
				Search(true);
			}
		}
		else if (Streq(buf, "stop\n")) {
			g_Stopped = true;
		}
		else if (Streq(buf, "i\n")) {
			EnterInteractiveMode();
			Stdout_wl("In uci mode");
		}
		else if (Streq(buf, "\n")) {
			
		}
		else {
			Stdout_wl("unknown command");
		}
		fgets(buf, UCI_BUF_SIZE, stdin);
	}
}

void PrintOptions() {
	printf("\nm: make move\n");
	printf("c: computer move\n");
	printf("t: run tests\n");
	printf("g: print game\n");
	printf("e: eval\n");
	printf("h: help\n");
	printf("q: quit\n");
}

int EnterInteractiveMode() {
	char buffer[20];
	char in = ' ';
	PrintGame(&g_mainGame);
	PrintOptions();

	while (in != 'q')
	{
		//system("@cls||clear");
		printf("h for help> ");
		fgets(buffer, 20, stdin);
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
			printf("Eval: %d\n", GetEval(&g_mainGame, 0));
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

void PutFreePieceAt(int square, enum PieceType pieceType, int side01) {
	for (int p = 0; p < 16; p++) {
		Piece* piece = &(g_mainGame.Pieces[side01][p]);
		if (piece->Off && piece->Type == pieceType) {
			piece->SquareIndex = square;
			piece->Off = false;
			return;
		}
	}
}

void InitPiece(int file, int rank, enum PieceType type, Side color) {
	g_mainGame.Squares[rank * 8 + file] = type | color;
	PutFreePieceAt(rank * 8 + file, type | color, color >> 4);
}

void StartPosition() {
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
}

char PieceChar(PieceType pieceType) {
	PieceType color = pieceType & (BLACK | WHITE);
	PieceType pt = pieceType & 7;
	switch (pt)
	{
	case PAWN:
		return color == WHITE ? 'P' : 'p';
	case ROOK:
		return color == WHITE ? 'R' : 'r';
	case KNIGHT:
		return color == WHITE ? 'N' : 'n';
	case BISHOP:
		return color == WHITE ? 'B' : 'b';
	case QUEEN:
		return color == WHITE ? 'Q' : 'q';
	case KING:
		return color == WHITE ? 'K' : 'k';
	default:
		return ' ';
	}
}

void PrintGame(Game* game) {
	// ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗
	char top[] = { 201,205,205,205,209,205,205,205,209,205,205,205,209,205,205,205,209,205,205,205,209,205,205,205,209,205,205,205,209,205,205,205,187,0 };
	// ╟───┼───┼───┼───┼───┼───┼───┼───╢
	char rowLine[] = { 199,196,196,196,197,196,196,196,197,196,196,196,197,196,196,196,197,196,196,196,197,196,196,196,197,196,196,196,197,196,196,196,182,0 };
	// ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝
	char lastLine[] = { 200,205,205,205, 207,205,205,205, 207,205,205,205, 207,205,205,205, 207,205,205,205, 207,205,205,205, 207,205,205,205, 207,205,205,205, 188, 0 };
	// ║
	char vBorder = 186; 

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
			char c[] = { PieceChar(piece), 0 };
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

PieceType parsePieceType(char c) {
	switch (c)
	{
	case 'p': return PAWN   | BLACK;
	case 'r': return ROOK   | BLACK;
	case 'b': return BISHOP | BLACK;
	case 'n': return KNIGHT | BLACK;
	case 'q': return QUEEN  | BLACK;
	case 'k': return KING   | BLACK;
	case 'P': return PAWN   | WHITE;
	case 'R': return ROOK   | WHITE;
	case 'B': return BISHOP | WHITE;
	case 'N': return KNIGHT | WHITE;
	case 'Q': return QUEEN  | WHITE;
	case 'K': return KING   | WHITE;
	default:
		return NOPIECE;
	}
}

PieceType parseSide(char c) {
	switch (c)
	{
	case 'w': return WHITE;
	case 'b': return BLACK;
	default:
		return NOPIECE;
	}
}

void InitScores() {
	g_mainGame.Material[0] = 0;
	g_mainGame.Material[1] = 0;
	g_mainGame.PositionScore = 0;

	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece piece = g_mainGame.Pieces[s][p];
			if (piece.Off)
				continue;
			int i = piece.SquareIndex;
			PieceType pt = piece.Type & 7;
			int colorSide = (piece.Type & (WHITE | BLACK)) >> 4;
			g_mainGame.Material[colorSide] += MaterialMatrix[colorSide][pt];
			g_mainGame.PositionScore += PositionValueMatrix[pt][colorSide][i];
		}
	}
	int endGame = g_mainGame.Material[1] - g_mainGame.Material[0] < ENDGAME ? 1 : 0;
	g_mainGame.PositionScore += KingPositionValueMatrix[endGame][0][g_mainGame.KingSquares[0]];
	g_mainGame.PositionScore += KingPositionValueMatrix[endGame][1][g_mainGame.KingSquares[1]];

}

void InitHash() {
	g_mainGame.Hash = StartHash;
	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece piece = g_mainGame.Pieces[s][p];
			if (piece.Off)
				continue;
			int i = piece.SquareIndex;
			g_mainGame.Hash ^= ZobritsPieceTypesSquares[piece.Type][i];
		}
	}
	g_mainGame.Hash ^= ZobritsSides[g_mainGame.Side01];
	if (g_mainGame.State & WhiteCanCastleLong)
		g_mainGame.Hash ^= ZobritsCastlingRights[0];
	if (g_mainGame.State & WhiteCanCastleShort)
		g_mainGame.Hash ^= ZobritsCastlingRights[1];
	if (g_mainGame.State & BlackCanCastleLong)
		g_mainGame.Hash ^= ZobritsCastlingRights[2];
	if (g_mainGame.State & BlackCanCastleShort)
		g_mainGame.Hash ^= ZobritsCastlingRights[3];

	g_mainGame.Hash ^= ZobritsEnpassantFile[g_mainGame.State & 15];
}

void ReadFen(char* fen) {
	InitPieceList();
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		g_mainGame.Squares[i] = NOPIECE;
	int index = 0;
	int file = 0;
	int rank = 7;
	while (fen[index] != ' ' && fen[index])
	{
		char c = fen[index];
		index++;
		if (isdigit(c)) {
			int dig = ParseChar(c);
			file += dig;
		}
		else if (c == '/') {
			rank--;
			file = 0;
		}
		else {
			PieceType pt = parsePieceType(c);
			InitPiece(file, rank, pt & 7, pt & 24);
			file++;
		}
	}

	index++;
	g_mainGame.Side = parseSide(fen[index]);
	g_mainGame.Side01 = g_mainGame.Side >> 4;
	index++;
	index++;
	g_mainGame.State = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': g_mainGame.State |= WhiteCanCastleShort;
			break;
		case 'Q': g_mainGame.State |= WhiteCanCastleLong;
			break;
		case 'k': g_mainGame.State |= BlackCanCastleShort;
			break;
		case 'q': g_mainGame.State |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		g_mainGame.State |= (enpFile + 1);
	//todo: counters	

	for (int i = 0; i < 64; i++)
	{
		if ((g_mainGame.Squares[i] & 7) == KING) {
			int color = (g_mainGame.Squares[i] >> 4);
			g_mainGame.KingSquares[color] = i;
		}
	}

	InitScores();
	InitHash();
}

void WriteFen(char* fenBuffer) {
	int index = 0;
	for (int rank = 8 - 1; rank >= 0; rank--)
	{
		for (int file = 0; file < 8; file++)
		{
			int emptyCount = 0;
			while (g_mainGame.Squares[rank * 8 + file] == NOPIECE && file < 8)
			{
				emptyCount++;
				file++;
			}

			if (emptyCount > 0) {
				fenBuffer[index++] = '0' + emptyCount;
				file--;
			}
			else {
				fenBuffer[index++] = PieceChar(g_mainGame.Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = g_mainGame.Side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (g_mainGame.State & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (g_mainGame.State & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (g_mainGame.State & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (g_mainGame.State & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';

	char noFile = 'a' - 1;
	char enPassantFile = (g_mainGame.State & 15) + noFile;
	if (enPassantFile == noFile)
		fenBuffer[index++] = '-';
	else
	{
		fenBuffer[index++] = enPassantFile;
		fenBuffer[index++] = g_mainGame.Side == WHITE ? '6' : '3';
	}
	fenBuffer[index] = '\0';
}

void AdjustPositionImportance() {
	for (int i = 1; i < 7; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			PositionValueMatrix[i][0][s] = PositionValueMatrix[i][0][s] / 3;
			PositionValueMatrix[i][1][s] = PositionValueMatrix[i][1][s] / 3;
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = KingPositionValueMatrix[0][0][i] / 3;
		KingPositionValueMatrix[1][0][i] = KingPositionValueMatrix[1][0][i] / 3;

		KingPositionValueMatrix[0][1][i] = KingPositionValueMatrix[0][1][i] / 3;
		KingPositionValueMatrix[1][1][i] = KingPositionValueMatrix[1][1][i] / 3;
	}
}

void SwitchSignOfWhitePositionValue() {
	for (int i = 1; i < 7; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			PositionValueMatrix[i][0][s] = -PositionValueMatrix[i][0][s];
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = -KingPositionValueMatrix[0][0][i];
		KingPositionValueMatrix[1][0][i] = -KingPositionValueMatrix[1][0][i];
	}
}

