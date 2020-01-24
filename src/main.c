#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <Windows.h>
#include <stdio.h>

#include "basic_structs.h"
#include "patterns.h"
#include "main.h"
#include "utils.h"
#include "tests.h"
#include "evaluation.h"
#include "sort.h"
#include "hashTable.h"
#include "timeControl.h"

Game g_threadGames[SEARCH_THREADS];
GlobalRootMoves g_rootMoves;
int g_LastStartedMove = -1;
HANDLE g_MutexFreeMove;
bool g_Stopped;

void ComputerMove() {
	g_topSearchParams.MoveTime = 5000;
	Move move = Search(false);
	int captIndex = MakeMove(move, &g_mainGame);
}

void ManualMove() {
	printf("\nYour move: ");
	char sMove[6];
	scanf_s(" %6c", sMove, 6);

	MakePlayerMove(sMove);
}

void DefaultSearch() {
	g_topSearchParams.BlackIncrement = 0;
	g_topSearchParams.BlackTimeLeft = 0;
	g_topSearchParams.MaxDepth = 30;
	g_topSearchParams.MoveTime = 0;
	g_topSearchParams.TimeControl = false;
	g_topSearchParams.WhiteIncrement = 0;
	g_topSearchParams.BlackIncrement = 0;
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
			g_mainGame.Pieces[s][p].Off = true;
	}
}

int main(int argc, char* argv[]) {
	g_MutexFreeMove = CreateMutex(NULL, FALSE, NULL);
	SwitchSignOfWhitePositionValue();
	DefaultSearch();
	ResetDepthTimes();
	AdjustPositionImportance();
	GenerateZobritsKeys();
	Allocate(1024);
	ClearHashTable();
	InitGame();
	printf("initialized\n");

	EnterUciMode();
	return 0;
}

void EnterUciMode() {
#define UCI_BUF_SIZE 5000
	char buf[UCI_BUF_SIZE];
	fgets(buf, 5000, stdin);
	while (!streq(buf, "quit\n"))
	{
		if (startsWith(buf, "ucinewgame")) {
			//ClearHashTable();  // the reason is that estimation of next move is much easier.
			ResetDepthTimes();
			stdout_wl("info string New game");
		}
		else if (startsWith(buf, "uci")) {
			stdout_wl("id name CChess");
			stdout_wl("id author Kristian Ekman");
			stdout_wl("option name Hash type spin default 1024 min 1 max 2048");
			stdout_wl("uciok");
		}
		else if (startsWith(buf, "isready")) {
			stdout_wl("readyok");
		}
		else if (startsWith(buf, "setoption name Hash value")) {
			//setoption name Hash value 32
			char* token = strtok(buf, " ");
			while (token != NULL) {
				if (streq(token, "value")) {
					token = strtok(NULL, " ");
					int mb = atoi(token);
					if (mb >= 1 && mb <= 2048) { // actual limit is 32760mb
						Allocate(mb);
						stdout_wl("info string New hash size set");
					}
					else {
						stdout_wl("info string Invalid Hash size value (1 - 2048)");
					}
					break;
				}
				token = strtok(NULL, " ");
			}
		}
		else if (startsWith(buf, "position ")) {
			//postion fen | moves
			InitGame();
			int movesPos = indexOf(buf, " moves");
			if (contains(buf, " fen "))
			{
				char* pFen = strstr(buf, " fen ");
				int fenPos = pFen - buf + 5;

				if (movesPos == -1) { // note sure if "moves" string is mandatory
					movesPos = strlen(buf) - 1;
				}

				int fenLength = movesPos - fenPos; //position fen xxx...yyy moves
				char fen[256];
				memcpy(fen, pFen + 5, fenLength);
				fen[fenLength + 1] = "\0";
				ReadFen(fen);
			}

			if (contains(buf, " moves "))
			{
				char* pMoves = strstr(buf, " moves ");
				char* token = strtok(pMoves, " ");
				while (token != NULL) {
					MakePlayerMove(token);
					token = strtok(NULL, " ");
				}
				printf("info string position parsed\n");
				AssertGame(&g_mainGame);
			}
		}
		else if (startsWith(buf, "go ")) {
			DefaultSearch();
			if (contains(buf, "infinite")) {
				g_topSearchParams.MaxDepth = 30;
				g_topSearchParams.TimeControl = false;
				Search(true);
			}
			// else search with time control
			// Sök i 1/50 av kvarstående tid i middle game (efter book opening)
			// I end game?
			else {
				char* token = strtok(buf, " ");
				while (token != NULL) {
					if (streq(token, "depth")) {
						char* depth = strtok(NULL, " ");
						sscanf(depth, "%d", &g_topSearchParams.MaxDepth);
					}
					else if (streq(token, "movetime")) {
						char* movetime = strtok(NULL, " ");
						sscanf(movetime, "%d", &g_topSearchParams.MoveTime);
					}
					else if (streq(token, "wtime")) {
						g_topSearchParams.TimeControl = true;
						char* wtime = strtok(NULL, " ");
						sscanf(wtime, "%d", &g_topSearchParams.WhiteTimeLeft);
						//go wtime 900000 btime 900000 winc 0 binc 0
					}
					else if (streq(token, "btime")) {
						g_topSearchParams.TimeControl = true;
						char* btime = strtok(NULL, " ");
						sscanf(btime, "%d", &g_topSearchParams.BlackTimeLeft);
					} if (streq(token, "winc")) {
						char* winc = strtok(NULL, " ");
						sscanf(winc, "%d", &g_topSearchParams.WhiteIncrement);
					} if (streq(token, "binc")) {
						char* binc = strtok(NULL, " ");
						sscanf(binc, "%d", &g_topSearchParams.BlackIncrement);
					}
					token = strtok(NULL, " ");
				}

				//Fallback if time control fails
				if (g_topSearchParams.TimeControl)
				{
					g_topSearchParams.MoveTime = g_topSearchParams.WhiteTimeLeft / 20;
					if (g_mainGame.Side == BLACK)
						g_topSearchParams.MoveTime = g_topSearchParams.BlackTimeLeft / 20;
				}
				printf("info string Searching Starting\n");
				fflush(stdout);
				Search(true);
			}
		}
		else if (streq(buf, "stop\n")) {
			g_Stopped = true;
		}
		else if (streq(buf, "i\n")) {
			EnterInteractiveMode();
		}
		else {
			stdout_wl("info string unknown command");
		}
		fgets(buf, UCI_BUF_SIZE, stdin);
	}
}

int EnterInteractiveMode() {
	char scan = 0;
	while (scan != 'q')
	{
		system("@cls||clear");
		PrintGame(&g_mainGame);
		printf("m: make move\n");
		printf("c: computer move\n");
		printf("t: run tests\n");
		printf("q: quit\n");
		scanf_s(" %c", &scan, 1);
		switch (scan)
		{
		case 'm':
			ManualMove();
			break;
		case 'c':
			ComputerMove();
			break;
		case 't':

			runAllTests();
			break;
		case 'q':
			break;
		default:
			break;
		}
		PrintGame(&g_mainGame);
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

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	g_mainGame.Squares[rank * 8 + file] = type | color;
	PutFreePieceAt(rank * 8 + file, type | color, color >> 4);
}

void InitGame() {
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
	printf("  ---------------------------------\n");

	for (int r = 8 - 1; r >= 0; r--)
	{
		printf("%d ", r + 1);
		for (int f = 0; f < 8; f++)
		{
			PieceType piece = game->Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
	printf("%llu\n", game->Hash);
	char fen[100];
	WriteFen(fen);
	printf("%s\n", fen);
}

void KingPositionScore(Move move, Game* game) {
	//aproximation that endgame starts att 1800 of total piece value, eg rook, knight, pawn per player
	int endGame = game->Material[1] - game->Material[0] < 1800 ? 1 : 0;
	game->PositionScore += KingPositionValueMatrix[endGame][game->Side >> 4][move.To];
	game->PositionScore -= KingPositionValueMatrix[endGame][game->Side >> 4][move.From];
}

int SetCaptureOff(Game* game, int side, int squareIndex) {
	for (int i = 0; i < 16; i++)
	{
		Piece* capture = &game->Pieces[side][i];
		if (capture->SquareIndex == squareIndex && !capture->Off) {
			capture->Off = true;
			return i;
		}
	}

	printf("Invalid SetCaptureOff parameters\n");
}

void MovePiece(Game* game, int side01, int from, int to) {
	for (int i = 0; i < 16; i++)
	{
		Piece* capture = &game->Pieces[side01][i];
		if (capture->SquareIndex == from && !capture->Off) {
			capture->SquareIndex = to;
			return;
		}
	}
	printf("Invalid MovePiece parameters\n");
}

void AssertGame(Game* game) {
#ifdef _DEBUG
	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece* piece = &game->Pieces[s][p];
			PieceType squareType = game->Squares[piece->SquareIndex];

			if (!piece->Off && squareType != piece->Type) {
				printf("Invalid game\n");
			}
		}
	}

	for (size_t i = 0; i < 64; i++)
	{
		PieceType pt = game->Squares[i];
		int side01 = (pt & 24) >> 4;
		bool found = false;
		for (size_t p = 0; p < 16; p++)
		{
			Piece* piece = &game->Pieces[side01][p];
			if (piece->Type == pt && !piece->Off) {
				found = true;
			}
		}
		if (pt != NOPIECE && !found) {
			printf("Invalid game. Square piec not found.\n");
		}
	}
#endif // _DEBUG
}

int MakeMove(Move move, Game* game) {
	char f = move.From;
	char t = move.To;

	PieceType captType = game->Squares[t];
	int captColor = captType >> 4;
	int side01 = game->Side >> 4;

	//removing piece from square removes its position score
	game->PositionScore -= PositionValueMatrix[captType & 7][captColor][t];

	PieceType pieceType = game->Squares[f];

	char pt = pieceType & 7;
	game->PositionScore -= PositionValueMatrix[pt][side01][f];
	game->PositionScore += PositionValueMatrix[pt][side01][t];

	game->Squares[t] = game->Squares[f];
	game->Squares[f] = NOPIECE;

	int captIndex = -1;
	if (captType && move.MoveInfo != EnPassantCapture)
	{
		captIndex = SetCaptureOff(game, !side01, t);
		game->Material[captColor] -= MaterialMatrix[captColor][captType & 7];
	}
	game->Pieces[side01][move.PieceIdx].SquareIndex = t;

	U64 hash = game->Hash;
	hash ^= ZobritsPieceTypesSquares[pieceType][f];
	hash ^= ZobritsPieceTypesSquares[pieceType][t];
	hash ^= ZobritsPieceTypesSquares[captType][t];

	hash ^= ZobritsEnpassantFile[game->State & 15];
	//resetting en passant every move
	game->State &= ~15;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game->Squares[t] = QUEEN | game->Side;
		game->Material[side01] += MaterialMatrix[side01][QUEEN + 6];
		hash ^= ZobritsPieceTypesSquares[QUEEN | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (QUEEN | game->Side);

		break;
	case PromotionRook:
		game->Squares[t] = ROOK | game->Side;
		game->Material[side01] += MaterialMatrix[side01][ROOK + 6];
		hash ^= ZobritsPieceTypesSquares[ROOK | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (ROOK | game->Side);

		break;
	case PromotionBishop:
		game->Squares[t] = BISHOP | game->Side;
		game->Material[side01] += MaterialMatrix[side01][BISHOP + 6];
		hash ^= ZobritsPieceTypesSquares[BISHOP | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (BISHOP | game->Side);

		break;
	case PromotionKnight:
		game->Squares[move.To] = KNIGHT | game->Side;
		game->Material[side01] += MaterialMatrix[side01][KNIGHT + 6];
		hash ^= ZobritsPieceTypesSquares[KNIGHT | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (KNIGHT | game->Side);

		break;
	case KingMove:
		game->KingSquares[side01] = t;
		if (game->Side == WHITE)
		{
			if (game->State & WhiteCanCastleLong)
				hash ^= ZobritsCastlingRights[0];
			if (game->State & WhiteCanCastleShort)
				hash ^= ZobritsCastlingRights[1];
		}
		else { //black
			if (game->State & BlackCanCastleLong)
				hash ^= ZobritsCastlingRights[2];
			if (game->State & BlackCanCastleShort)
				hash ^= ZobritsCastlingRights[3];
		}
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.

		KingPositionScore(move, game);
		break;
	case RookMove:
		switch (move.From)
		{
		case 0:
			if (game->State & WhiteCanCastleLong) {
				game->State &= ~WhiteCanCastleLong;
				hash ^= ZobritsCastlingRights[0];
			}
			break;
		case 7:
			if (game->State & WhiteCanCastleShort) {
				game->State &= ~WhiteCanCastleShort;
				hash ^= ZobritsCastlingRights[1];
			}
			break;
		case 56:
			if (game->State & BlackCanCastleLong) {
				game->State &= ~BlackCanCastleLong;
				hash ^= ZobritsCastlingRights[2];
			}
			break;
		case 63:
			if (game->State & BlackCanCastleShort) {
				game->State &= ~BlackCanCastleShort;
				hash ^= ZobritsCastlingRights[3];
			}
			break;
		default:
			break;
		}
		break;
	case CastleShort:
	{
		game->KingSquares[side01] = t;
		char rookFr = 7 + CastlesOffset[side01];
		char rookTo = 5 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = rook;
		MovePiece(game, side01, rookFr, rookTo);

		game->PositionScore += CastlingPoints[side01];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.
		hash ^= ZobritsCastlingRights[side01 * 2];
		hash ^= ZobritsCastlingRights[side01 * 2 + 1];
	}
	break;
	case CastleLong:
	{
		game->KingSquares[side01] = t;
		char rookFr = CastlesOffset[side01];
		char rookTo = 3 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = ROOK | game->Side;
		MovePiece(game, side01, rookFr, rookTo);

		game->PositionScore += CastlingPoints[side01];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.
		hash ^= ZobritsCastlingRights[side01 * 2];
		hash ^= ZobritsCastlingRights[side01 * 2 + 1];
	}
	break;
	case EnPassant:
		game->State |= ((f & 7) + 1); //Sets the file. a to h. File is 1 to 8.
		hash ^= ZobritsEnpassantFile[(f & 7) + 1];
		break;
	case EnPassantCapture:
	{
		char behind = t + Behind[side01];
		game->Squares[behind] = NOPIECE;
		captIndex = SetCaptureOff(game, !side01, behind);
		game->Material[side01] += MaterialMatrix[side01][PAWN];
		hash ^= ZobritsPieceTypesSquares[PAWN | captColor][behind];
	}
	break;
	default:
		break;
	}

	hash ^= ZobritsSides[side01];
	game->Hash = hash;
	game->Side ^= 24;
	game->PositionHistory[game->PositionHistoryLength++] = game->Hash;
	AssertGame(game);
	return captIndex;
}

void UnMakeMove(Move move, int captIndex, GameState prevGameState, short prevPositionScore, Game* game, U64 prevHash) {

	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;

	PieceType capture = NOPIECE;
	if (captIndex != -1) {
		capture = game->Pieces[!otherSide01][captIndex].Type;
	}
	game->Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];

	game->Squares[move.From] = game->Squares[move.To];
	if (move.MoveInfo != EnPassantCapture)
		game->Squares[move.To] = capture;

	game->Pieces[otherSide01][move.PieceIdx].SquareIndex = move.From;
	if (capture)
		game->Pieces[!otherSide01][captIndex].Off = false;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][QUEEN + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case PromotionRook:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][ROOK + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);

		break;
	case PromotionBishop:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][BISHOP + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case PromotionKnight:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][KNIGHT + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case KingMove:
		game->KingSquares[otherSide01] = move.From;
		break;
	case CastleShort:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[5 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[7 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		MovePiece(game, otherSide01, 5 + CastlesOffset[otherSide01], 7 + CastlesOffset[otherSide01]);
		break;
	case CastleLong:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[3 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[0 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		MovePiece(game, otherSide01, 3 + CastlesOffset[otherSide01], 0 + CastlesOffset[otherSide01]);
		break;
	case EnPassantCapture:
		game->Squares[move.To + Behind[otherSide01]] = PAWN | game->Side;
		game->Squares[move.To] = NOPIECE;
		// captured piece should be put back earlier
		break;
	default:
		break;
	}
	game->State = prevGameState;
	game->PositionScore = prevPositionScore;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->PositionHistoryLength--;
	AssertGame(game);
}

void MakeNullMove(Game* game) {
	int side01 = game->Side >> 4;
	U64 hash = ZobritsEnpassantFile[game->State & 15];
	//resetting en passant
	game->State &= ~15;

	hash ^= ZobritsSides[side01];
	game->Hash ^= hash;
	game->Side ^= 24;
	game->PositionHistory[game->PositionHistoryLength++] = game->Hash;
}

void UnMakeNullMove(GameState prevGameState, Game* game, U64 prevHash) {
	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;
	game->State = prevGameState;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->PositionHistoryLength--;
}

bool SquareAttacked(int square, Side attackedBy, Game* game) {
	int side01 = attackedBy >> 4;
	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;
		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int captPat = PawnCapturePattern[side01];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			char raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rr = 1; rr <= rayLength; rr++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
					if (toSquare == square)
						return true;
					if (game->Squares[toSquare] > NOPIECE)
						break;
				}
			}
			break;
		}
		}
	}
	return false;
}

void SortMoves(Move* moves, int moveCount, Side side) {

	if (side == WHITE)
		QuickSort(moves, 0, moveCount - 1);
	else
		QuickSortDescending(moves, 0, moveCount - 1);
}

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo, Game* game, char pieceIdx) {
	GameState prevGameState = game->State;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	move.PieceIdx = pieceIdx;
	short prevPosScore = game->PositionScore;
	U64 prevHash = game->Hash;

	int captIndex = MakeMove(move, game);
	move.ScoreAtDepth = GetMoveScore(game);
	//move.ScoreAtDepth = GetEval(game, move.ScoreAtDepth);
	game->MovesBuffer[game->MovesBufferLength++] = move;

	UnMakeMove(move, captIndex, prevGameState, prevPosScore, game, prevHash);
}

void CreateMoves(Game* game, int depth) {
	game->MovesBufferLength = 0;
	int side01 = game->Side >> 4;
	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;
		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int pat = PawnPattern[game->Side >> 4];
			int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
			for (int pp = 1; pp <= pawnPatLength; pp++)
			{
				int toSquare = PieceTypeSquarePatterns[pat][i][pp];
				if (game->Squares[toSquare] != NOPIECE)
					break;
				if (toSquare < 8 || toSquare > 55) {
					CreateMove(i, toSquare, PromotionQueen, game, pi);
					CreateMove(i, toSquare, PromotionRook, game, pi);
					CreateMove(i, toSquare, PromotionBishop, game, pi);
					CreateMove(i, toSquare, PromotionKnight, game, pi);
				}
				else if (pp == 2) {
					CreateMove(i, toSquare, EnPassant, game, pi);
				}
				else {
					CreateMove(i, toSquare, PlainMove, game, pi);
				}
			}

			int captPat = PawnCapturePattern[game->Side >> 4];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				//Must be a piece of opposite color.
				if (game->Squares[toSquare] & (game->Side ^ 24))
				{
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen, game, pi);
						CreateMove(i, toSquare, PromotionRook, game, pi);
						CreateMove(i, toSquare, PromotionBishop, game, pi);
						CreateMove(i, toSquare, PromotionKnight, game, pi);
					}
					else {
						CreateMove(i, toSquare, PlainMove, game, pi);
					}
				}
				else {
					int enpFile = (game->State & 15) - 1;
					if (enpFile > -1) {
						int toFile = toSquare & 7;
						int toRank = toSquare >> 3;
						if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side >> 4])
							CreateMove(i, toSquare, EnPassantCapture, game, pi);
					}
				}
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (!(game->Squares[toSquare] & game->Side)) {
					CreateMove(i, toSquare, 0, game, pi);
				}
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (!(game->Squares[toSquare] & game->Side)) {
					CreateMove(i, toSquare, KingMove, game, pi);
				}
			}

			int castleBlackOffset = CastlesOffset[game->Side >> 4];
			if (i == castleBlackOffset + 4) { //King on origin pos
				if ((game->Side & WHITE && game->State & WhiteCanCastleShort) || (game->Side & BLACK && game->State & BlackCanCastleShort)) {
					if ((game->Squares[castleBlackOffset + 7] & 7) == ROOK &&
						game->Squares[castleBlackOffset + 5] == NOPIECE &&
						game->Squares[castleBlackOffset + 6] == NOPIECE)
					{
						if (!SquareAttacked(5 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game))
							CreateMove(i, 6 + castleBlackOffset, CastleShort, game, pi);
					}
				}
				if ((game->Side & WHITE && game->State & WhiteCanCastleLong) || (game->Side & BLACK && game->State & BlackCanCastleLong)) {
					if ((game->Squares[castleBlackOffset] & 7) == ROOK &&
						game->Squares[castleBlackOffset + 1] == NOPIECE &&
						game->Squares[castleBlackOffset + 2] == NOPIECE &&
						game->Squares[castleBlackOffset + 3] == NOPIECE)
					{
						if (!SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(3 + castleBlackOffset, game->Side ^ 24, game))
							CreateMove(i, 2 + castleBlackOffset, CastleLong, game, pi);
					}
				}
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rnd_seed = 1; rnd_seed <= rayLength; rnd_seed++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rnd_seed];
					PieceType toPiece = game->Squares[toSquare];
					MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;

					if (toPiece != NOPIECE) {
						if (!(toPiece & game->Side)) {
							CreateMove(i, toSquare, moveInfo, game, pi);
						}
						break;
					}
					else {
						CreateMove(i, toSquare, moveInfo, game, pi);
					}
				}
			}
			break;
		}
		}

	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game->Side);
}

void CreateCaptureMoves(Game* game) {
	game->MovesBufferLength = 0;
	int side01 = game->Side >> 4;
	char otherSide = game->Side ^ 24;

	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;

		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int infront = -Behind[side01];
			int toSquare = i + infront;
			if (game->Squares[toSquare] == NOPIECE)
			{
				if (toSquare < 8 || toSquare > 55) {
					CreateMove(i, toSquare, PromotionQueen, game, pi);
					/*CreateMove(i, toSquare, PromotionRook, game, pi);
					CreateMove(i, toSquare, PromotionBishop, game, pi);
					CreateMove(i, toSquare, PromotionKnight, game, pi);*/
				}
			}

			int captPat = PawnCapturePattern[side01];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				//Must be a piece of opposite color.
				if (game->Squares[toSquare] & otherSide)
				{
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen, game, pi);
						/*CreateMove(i, toSquare, PromotionRook, game, pi);
						CreateMove(i, toSquare, PromotionBishop, game, pi);
						CreateMove(i, toSquare, PromotionKnight, game, pi);*/
					}
					else {
						CreateMove(i, toSquare, PlainMove, game, pi);
					}
				}
				else {
					int enpFile = (game->State & 15) - 1;
					if (enpFile > -1) {
						int toFile = toSquare & 7;
						int toRank = toSquare >> 3;
						if (toFile == enpFile && toRank == EnpassantRankPattern[side01])
							CreateMove(i, toSquare, EnPassantCapture, game, pi);
					}
				}
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (game->Squares[toSquare] & otherSide) {
					CreateMove(i, toSquare, 0, game, pi);
				}
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (game->Squares[toSquare] & otherSide) {
					CreateMove(i, toSquare, KingMove, game, pi);
				}
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rr = 1; rr <= rayLength; rr++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
					PieceType toPiece = game->Squares[toSquare];
					MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;
					if (toPiece & otherSide) {
						CreateMove(i, toSquare, moveInfo, game, pi);
						break;
					}
				}
			}
			break;
		}
		}
	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game->Side);
}

PieceType parsePieceType(char c) {
	switch (c)
	{
	case 'p': return PAWN | BLACK;
	case 'r': return ROOK | BLACK;
	case 'b': return BISHOP | BLACK;
	case 'n': return KNIGHT | BLACK;
	case 'q': return QUEEN | BLACK;
	case 'k': return KING | BLACK;
	case 'P': return PAWN | WHITE;
	case 'R': return ROOK | WHITE;
	case 'B': return BISHOP | WHITE;
	case 'N': return KNIGHT | WHITE;
	case 'Q': return QUEEN | WHITE;
	case 'K': return KING | WHITE;

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

Move ParseMove(char* sMove, MoveInfo info) {
	int fromFile = sMove[0] - 'a';
	int fromRank = sMove[1] - '1';
	int toFile = sMove[2] - 'a';
	int toRank = sMove[3] - '1';
	Move move;
	move.From = fromRank * 8 + fromFile;
	move.To = toRank * 8 + toFile;
	move.MoveInfo = info;
	return move;
}

void InitScores() {
	g_mainGame.Material[0] = 0;
	g_mainGame.Material[1] = 0;
	g_mainGame.PositionScore = 0;

	for (int i = 0; i < 64; i++)
	{
		PieceType pt = g_mainGame.Squares[i] & 7;
		int colorSide = (g_mainGame.Squares[i] & (WHITE | BLACK)) >> 4;
		g_mainGame.Material[colorSide] += MaterialMatrix[colorSide][pt];
		g_mainGame.PositionScore += PositionValueMatrix[pt][colorSide][i];
	}

	//aproximation that endgame starts att 1900.
	int endGame = g_mainGame.Material[1] - g_mainGame.Material[0] < 1900 ? 1 : 0;

	g_mainGame.PositionScore += KingPositionValueMatrix[endGame][0][g_mainGame.KingSquares[0]];
	g_mainGame.PositionScore += KingPositionValueMatrix[endGame][1][g_mainGame.KingSquares[1]];
}

void InitHash() {
	g_mainGame.Hash = 0;
	for (int i = 0; i < 64; i++)
		g_mainGame.Hash ^= ZobritsPieceTypesSquares[g_mainGame.Squares[i]][i];
	g_mainGame.Hash ^= ZobritsSides[g_mainGame.Side >> 4];
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
			int dig = parseChar(c);
			file += dig;
		}
		else if (c == '/') {
			rank--;
			file = 0;
		}
		else {
			PieceType pt = parsePieceType(c);
			InitPiece(file, rank, pt & 7, pt & 24);
			//mainGame.Squares[rank * 8 + file] = pt;

			file++;
		}
	}

	index++;
	g_mainGame.Side = parseSide(fen[index]);
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

void MoveToString(Move move, char sMove[5]) {
	char fromFile = (move.From & 7) + 'a';
	char fromRank = (move.From >> 3) + '1';
	char toFile = (move.To & 7) + 'a';
	char toRank = (move.To >> 3) + '1';
	sMove[0] = fromFile;
	sMove[1] = fromRank;
	sMove[2] = toFile;
	sMove[3] = toRank;
	sMove[4] = '\0';
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

void RemoveInvalidMoves(Game* game) {
	int validMovesCount = 0;
	Move validMoves[100];

	for (size_t m = 0; m < game->MovesBufferLength; m++)
	{
		Move move = game->MovesBuffer[m];
		GameState prevState = game->State;
		short prevPosScor = game->PositionScore;
		U64 prevHash = game->Hash;
		int captIndex = MakeMove(move, game);
		int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];

		bool legal = !SquareAttacked(kingSquare, game->Side, game);
		UnMakeMove(move, captIndex, prevState, prevPosScor, game, prevHash);
		if (legal)
			validMoves[validMovesCount++] = move;
	}
	memcpy(game->MovesBuffer, &validMoves, validMovesCount * sizeof(Move));
	game->MovesBufferLength = validMovesCount;
}

int ValidMoves(Move* moves) {
	CreateMoves(&g_mainGame, 0);
	RemoveInvalidMoves(&g_mainGame);

	if (g_mainGame.MovesBufferLength == 0)
		return 0;

	memcpy(moves, g_mainGame.MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	return g_mainGame.MovesBufferLength;
}

int ValidMovesOnThread(Game* game, Move* moves) {
	CreateMoves(game, 0);
	RemoveInvalidMoves(game);

	if (game->MovesBufferLength == 0)
		return 0;

	memcpy(moves, game->MovesBuffer, game->MovesBufferLength * sizeof(Move));
	return game->MovesBufferLength;
}

PlayerMove MakePlayerMoveOnThread(Game* game, char* sMove) {
	Move move = ParseMove(sMove, 0);
	Move validMoves[256];
	int length = ValidMovesOnThread(game, validMoves);
	PlayerMove playerMove;
	for (int i = 0; i < length; i++)
	{
		if (validMoves[i].From == move.From && validMoves[i].To == move.To) {
			playerMove.Move = validMoves[i];
			playerMove.PreviousGameState = game->State;
			playerMove.Invalid = false;
			playerMove.PreviousPositionScore = game->PositionScore;
			playerMove.PreviousHash = game->Hash;

			int captIndex = MakeMove(validMoves[i], game);
			playerMove.CaptureIndex = captIndex;
			return playerMove;
		}
	}
	playerMove.Invalid = true;
	return playerMove;
}

PlayerMove MakePlayerMove(char* sMove) {
	return MakePlayerMoveOnThread(&g_mainGame, sMove);
}

void UnMakePlayerMove(PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.CaptureIndex, playerMove.PreviousGameState, playerMove.PreviousPositionScore, &g_mainGame, playerMove.PreviousHash);
}

void UnMakePlayerMoveOnThread(Game* game, PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.CaptureIndex, playerMove.PreviousGameState, playerMove.PreviousPositionScore, game, playerMove.PreviousHash);
}

short TotalMaterial(Game* game) {
	return game->Material[0] + game->Material[1];
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

void MoveToTop(Move move, Move* list, int length) {
	for (size_t i = 0; i < length; i++)
	{
		if (move.From == list[i].From && move.To == list[i].To && i > 0) {
			Move temp = list[i];
			memmove(&list[1], list, i * sizeof(Move));
			list[0] = temp;
			return;
		}
	}
}

short AlphaBetaQuite(short alpha, short beta, Game* game, short moveScore) {

	g_SearchedNodes++;

	if (DrawByRepetition(game))
		return 0;

	int score = GetEval(game, moveScore); // There seems to be a small advantage in taking time to fully evaluate even here.

	if (game->Side == BLACK) {
		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;
	}
	else {
		if (score <= alpha)
			return alpha;
		if (score < beta)
			beta = score;
	}


	CreateCaptureMoves(game);
	int moveCount = game->MovesBufferLength;
	if (moveCount == 0)
		return score;

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));
	if (game->Side == BLACK) { //maximizing
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.ScoreAtDepth);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
			if (score > alpha) {
				if (score >= beta) {
					free(localMoves);
					return beta;
				}
				alpha = score;
			}
		}
		free(localMoves);
		return alpha;
	}
	else { //minimizing
		score = MAX_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.ScoreAtDepth);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
			if (score < beta) {
				if (score <= alpha) {
					free(localMoves);
					return alpha;
				}
				beta = score;
			}
		}
		free(localMoves);
		return beta;
	}
}

MoveKillersToTop(Game* game, Move* moveList, int moveListLength, int deep_in) {
	Move secondKiller = game->KillerMoves[deep_in][1];
	for (size_t i = 0; i < moveListLength; i++)
	{
		if (moveList[i].From == secondKiller.From && moveList[i].To == secondKiller.To) {
			Move temp = moveList[i];
			memmove(&moveList[1], moveList, i * sizeof(Move));
			moveList[0] = temp;
			break;
		}
	}
	Move firstKiller = game->KillerMoves[deep_in][1];
	for (size_t i = 0; i < moveListLength; i++)
	{
		if (moveList[i].From == firstKiller.From && moveList[i].To == firstKiller.To) {
			Move temp = moveList[i];
			memmove(&moveList[1], moveList, i * sizeof(Move));
			moveList[0] = temp;
			break;
		}
	}
}

short AlphaBeta(short alpha, short beta, int depth, int captIndex, Game* game, bool doNull, short moveScore, int deep_in) {
	if (g_Stopped)
		return moveScore; // should not be used;

	g_SearchedNodes++;

	if (!depth) {
		return AlphaBetaQuite(alpha, beta, game, moveScore);
	}

	if (DrawByRepetition(game))
		return 0;

	//In check extension
	int side01 = game->Side >> 4;
	int otherSide = game->Side ^ 24;
	bool incheck = SquareAttacked(game->KingSquares[side01], otherSide, game);
	if (incheck)
		depth++;

	//Probe hash
	short score = 0; Move pvMove;
	pvMove.MoveInfo = NotAMove;
	if (getScoreFromHash(game->Hash, depth, &score, &pvMove, alpha, beta)) {
		return score;
	}

	//NULL move check
	int r = 3; //todo: tests att sätta till 
	if ((game->Side == WHITE && game->Material[side01] < -500) || // todo: check for pieces when piece list works
		(game->Side == BLACK && game->Material[side01] > 500))
	{
		if (doNull && !incheck && game->PositionHistoryLength && depth >= r) {
			GameState prevState = game->State;
			U64 prevHash = game->Hash;
			MakeNullMove(game);
			if (game->Side == BLACK) {
				int nullScore = AlphaBeta(alpha, alpha + 1, depth - r, captIndex, game, false, moveScore, deep_in + 1);
				if (nullScore <= alpha && nullScore > -8000 && nullScore < 8000) {
					UnMakeNullMove(prevState, game, prevHash);
					return alpha;
				}
			}
			else //(game->Side == WHITE)
			{
				int nullScore = AlphaBeta(beta - 1, beta, depth - r, captIndex, game, false, moveScore, deep_in + 1);
				if (nullScore >= beta && nullScore > -8000 && nullScore < 8000) {
					UnMakeNullMove(prevState, game, prevHash);
					return beta;
				}
			}
			UnMakeNullMove(prevState, game, prevHash);
		}
	}

	//Move generation
	CreateMoves(game, depth);
	int moveCount = game->MovesBufferLength;

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));

	MoveKillersToTop(game, localMoves, moveCount, deep_in);

	if (pvMove.MoveInfo != NotAMove) {
		MoveToTop(pvMove, localMoves, moveCount);
	}

	// alpha beta pruning
	short bestScore = 0;
	int legalCount = 0;
	short oldAlpha = alpha;
	short oldBeta = beta;
	Move bestMove;
	if (game->Side == BLACK) { //maximizing, black
		bestScore = MIN_SCORE;
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);

			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			legalCount++;
			score = AlphaBeta(alpha, beta, depth - 1, captIndex, game, true, childMove.ScoreAtDepth, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);

			if (score > bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score > alpha) {
					if (score >= beta) {
						addHashScore(game->Hash, beta, depth, BETA, childMove.From, childMove.To);
						free(localMoves);

						if (captIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}
						return beta;
					}
					alpha = score;
				}
			}
		}

		free(localMoves);
		if (legalCount == 0) {
			if (incheck)
				return -8000 + deep_in;
			else
				return 0;
		}

		if (g_Stopped)
			return alpha;

		if (alpha != oldAlpha)
			addHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			addHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);

		return alpha;
	}
	else { //minimizing, white
		bestScore = MAX_SCORE;
		score = MAX_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			short prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;
			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			legalCount++;
			score = AlphaBeta(alpha, beta, depth - 1, captIndex, game, true, childMove.ScoreAtDepth, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);

			if (score < bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score < beta) {
					if (score <= alpha) {
						addHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);
						if (captIndex != -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}
						free(localMoves);
						return alpha;
					}
					beta = score;
				}
			}
		}
		free(localMoves);
		if (legalCount == 0) {
			if (incheck)
				return 8000 - deep_in; //mate
			else
				return 0; //stale mate
		}

		if (g_Stopped)
			return beta;

		if (beta != oldBeta)
			addHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			addHashScore(game->Hash, beta, depth, BETA, bestMove.From, bestMove.To);
		return beta;
	}
}

Game* CopyMainGame(int threadNo) {
	g_threadGames[threadNo] = g_mainGame;
	g_threadGames[threadNo].KingSquares[0] = g_mainGame.KingSquares[0];
	g_threadGames[threadNo].KingSquares[1] = g_mainGame.KingSquares[1];
	g_threadGames[threadNo].Material[0] = g_mainGame.Material[0];
	g_threadGames[threadNo].Material[1] = g_mainGame.Material[1];
	g_threadGames[threadNo].ThreadIndex = threadNo;

	memcpy(g_mainGame.MovesBuffer, g_threadGames[threadNo].MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	memcpy(g_mainGame.Squares, g_threadGames[threadNo].Squares, 64 * sizeof(PieceType));
	memcpy(g_mainGame.PositionHistory, g_threadGames[threadNo].PositionHistory, g_mainGame.PositionHistoryLength * sizeof(U64));
	memcpy(g_mainGame.Pieces, g_threadGames[threadNo].Pieces, 32 * sizeof(Piece));
	memset(g_threadGames[threadNo].KillerMoves, 0, 2 * 100 * sizeof(Move));
	return &g_threadGames[threadNo];
}

DWORD WINAPI DoNothingThread(int* prm) {
	Sleep(50);
	ExitThread(0);
}

int GetNextFreeMove() {
	DWORD waitResult = WaitForSingleObject(g_MutexFreeMove, INFINITE);
	int ret;
	if (waitResult == WAIT_OBJECT_0) {
		ret = g_LastStartedMove + 1;
		g_LastStartedMove++;
		ReleaseMutex(g_MutexFreeMove);
	}
	else {
		printf("Unexpected waitResult (%d) in GetNextFreeMove.", waitResult);
		exit(999);
	}
	return ret;
}

// Entry point for a thread that ttarts the alphabeta tree search for a given depth and a given move.
// When finished takes next root move until they are no more.
// Sets the score on the root move pointer. They are all common for all threads.
DWORD WINAPI SearchThread(ThreadParams* prm) {
	int moveIndex = GetNextFreeMove();
	do
	{
		Sleep(10);
		//if (prm->depth > 7)
			//printf("Start Thread %d on move %d\n", prm->threadID, moveIndex);

		Game* game = &(g_threadGames[prm->threadID]);
		Move move = g_rootMoves.moves[moveIndex];
		GameState gameState = game->State;
		int positionScore = game->PositionScore;
		U64 prevHash = game->Hash;

		int captIndex = MakeMove(move, game);
		short g_alpha = MIN_SCORE;
		short g_beta = MAX_SCORE;
		int score = AlphaBeta(g_alpha, g_beta, prm->depth, captIndex, game, true, move.ScoreAtDepth, 0);

		if (!g_Stopped)
			g_rootMoves.moves[moveIndex].ScoreAtDepth = score;
		UnMakeMove(move, captIndex, gameState, positionScore, game, prevHash);

		//if (prm->depth > 7)
			//printf("Stop  Thread %d on move %d\n", prm->threadID, moveIndex);

		moveIndex = GetNextFreeMove();

	} while (moveIndex < prm->moveCount);
	ExitThread(0);
	return 0;
}

void SetMovesScoreAtDepth(int depth, int moveCount) {

	int moveIndex = 0;
	//Startar alla trådar för angivet djup.
	//En tråd per root move. när den är klar tar den nästa lediga root move.
	//När alla trådar är klara sorteras root moves oc det bästa skrivs ut.

	HANDLE threadHandles[SEARCH_THREADS];

	ThreadParams* tps = malloc(sizeof(ThreadParams) * SEARCH_THREADS);

	g_LastStartedMove = -1;

	for (int i = 0; i < SEARCH_THREADS; i++)
		CopyMainGame(i);

	for (int i = 0; i < SEARCH_THREADS; i++)
	{
		if (i > moveCount - 1) //in case more threads than moves
			threadHandles[i] = CreateThread(NULL, 0, DoNothingThread, NULL, 0, NULL);
		else {
			tps->threadID = i;
			tps->depth = depth;
			tps->moveCount = moveCount;
			threadHandles[i] = CreateThread(NULL, 0, SearchThread, tps, 0, NULL);
			tps++;

		}
	}
	WaitForMultipleObjects(SEARCH_THREADS, threadHandles, TRUE, INFINITE);
	SortMoves(g_rootMoves.moves, moveCount, g_mainGame.Side); //TODO: Why?
	//free(tps);
}
// Background thread that sets Stopped flag after specified time in ms.
DWORD WINAPI TimeLimitWatch(void* args) {
	int ms = g_topSearchParams.MoveTime;
	clock_t start = clock();
	clock_t now = clock();
	printf("TimeLimitWatch %d\n", ms);
	while (!g_Stopped)
	{
		Sleep(100);
		now = clock();
		if ((now - start > (ms / (float)1000)* CLOCKS_PER_SEC))
		{
			printf("Search stopped by timeout.\n"); // This is not preferable since it is more economic to stop before going to next depth.
			fflush(stdout);
			break;
		}
	}

	g_Stopped = true;
	ExitThread(0);
	return 0;
}

// Starting point of a search for best move.
// Continues until time millis is reached or depth is reached.
// When async is set the result is printed to stdout. Not returned.
Move Search(bool async) {
	HANDLE timeLimitThread = 0;
	if (g_topSearchParams.MoveTime > 0) {
		timeLimitThread = CreateThread(NULL, 0, TimeLimitWatch, NULL, 0, NULL);
	}

	g_Stopped = false;
	g_SearchedNodes = 0;

	HANDLE handle = CreateThread(NULL, 0, BestMoveDeepening, NULL, 0, NULL);
	if (!async)
	{
		WaitForSingleObject(handle, INFINITE);
		if (timeLimitThread != 0)
			TerminateThread(timeLimitThread, 0);
		return g_topSearchParams.BestMove;
	}

	//this will not be used.
	Move nomove;
	nomove.MoveInfo = NotAMove;
	return nomove;
}

int PrintBestLine(Move move, int depth, float ellapsed) {
	char buffer[2000];
	char* pv = &buffer;
	char sMove[5];
	MoveToString(move, sMove);
	strcpy(pv, sMove);
	pv += 4;
	strcpy(pv, " ");
	pv++;
	Game* game = &g_mainGame;

	PlayerMove bestPlayerMove = MakePlayerMoveOnThread(game, sMove);
	int index = 0;
	PlayerMove moves[400];
	int movesCount = 0;
	while (movesCount < depth)
	{
		Move bestMove;
		if (!getBestMoveFromHash(game->Hash, &bestMove))
			break;

		char* sMove[5];
		MoveToString(bestMove, sMove);

		PlayerMove plMove = MakePlayerMoveOnThread(game, sMove);
		if (plMove.Invalid)
			break;
		moves[movesCount++] = plMove;
		strcpy(pv, sMove); pv += 4;
		strcpy(pv, " "); pv++;
	}
	strcpy(pv, "\0");

	for (int i = movesCount - 1; i >= 0; i--)
		UnMakePlayerMoveOnThread(game, moves[i]);
	UnMakePlayerMoveOnThread(game, bestPlayerMove);
	int nps = (float)g_SearchedNodes / ellapsed;
	int time = ellapsed * 1000;
	short score = move.ScoreAtDepth;
	if (game->Side == WHITE)
		score = -score;
	printf("info score cp %d depth %d nodes %d time %d nps %d pv %s\n", score, depth, g_SearchedNodes, time, nps, buffer);
	fflush(stdout);
	return 0;
}

// Starting point of one thread that evaluates best score for every 7th root move. (If there are 7 threads)
// Increasing depth until given max depth.
DWORD WINAPI  BestMoveDeepening(void* v) {
	int maxDepth = g_topSearchParams.MaxDepth;
	clock_t start = clock();
	//ClearHashTable();
	CreateMoves(&g_mainGame, 0);
	RemoveInvalidMoves(&g_mainGame);
	int moveCount = g_mainGame.MovesBufferLength;
	g_rootMoves.Length = moveCount;
	memcpy(g_rootMoves.moves, g_mainGame.MovesBuffer, moveCount * sizeof(Move));

	int depth = 1;
	char bestMove[5];
	int bestScore;
	do
	{
		clock_t depStart = clock();
		SetMovesScoreAtDepth(depth, moveCount);
		if (!g_Stopped) { // avbrutna depths ger felaktigt resultat.
			g_topSearchParams.BestMove = g_rootMoves.moves[0];
			bestScore = g_rootMoves.moves[0].ScoreAtDepth;
			MoveToString(g_rootMoves.moves[0], bestMove);
			depth++;
			float ellapsed = (float)(clock() - start) / CLOCKS_PER_SEC;
			PrintBestLine(g_rootMoves.moves[0], depth, ellapsed);
			if ((g_mainGame.Side == WHITE && bestScore < -7000) || (g_mainGame.Side == BLACK && bestScore > 7000))
			{
				g_Stopped = true;
				break; //A check mate is found, no need to search further.
			}

			float depthTime = (float)(clock() - depStart) / CLOCKS_PER_SEC;
			int moveNo = g_mainGame.PositionHistoryLength;

			RegisterDepthTime(moveNo, depth, depthTime * 1000);
			if (g_topSearchParams.TimeControl && !SearchDeeper(depth, moveNo, ellapsed * 1000, g_mainGame.Side)) {
				g_Stopped = true;
				break;
			}
		}

	} while (depth <= maxDepth && !g_Stopped);

	printf("bestmove %s\n", bestMove);
	fflush(stdout);

	ExitThread(0);
	return 0;
}
