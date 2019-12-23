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
#include "bestMovesTable.h"

Game mainGame;
Game threadGames[SEARCH_THREADS];
BestMovesTable bmTables[SEARCH_THREADS];
const int TBL_SIZE_MB = 32;

bool Stopped;

void computerMove() {
	Move move = Search(5, 0, false);
	MakeMove(move, &mainGame);
}

void manualMove() {
	printf("\nYour move: ");
	char sMove[6];
	scanf_s(" %6c", sMove, 6);

	MakePlayerMove(sMove);
}

int main(int argc, char* argv[]) {
	SwitchSignOfWhitePositionValue();
	//AdjustPositionImportance();
	GenerateZobritsKeys();
	ClearHashTable();
	InitGame();
	for (int i = 0; i < SEARCH_THREADS; i++)
		InitBestMovesTable(&bmTables[i], TBL_SIZE_MB);
	printf("initialized\n");

	EnterUciMode();
	return 0;
}

void EnterUciMode() {
	char buf[1024];
	fgets(buf, 1024, stdin);
	while (!streq(buf, "quit\n"))
	{
		if (startsWith(buf, "uci")) {
			stdout_wl("id name CChess");
			stdout_wl("id author Kristian Ekman");
			stdout_wl("uciok");
		}
		else if (startsWith(buf, "isready")) {
			stdout_wl("readyok");
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
			}
		}
		else if (startsWith(buf, "go ")) {
			if (contains(buf, " movetime ")) {
				int idx = indexOf(buf, " movetime ");
				idx += 10;
				char* sTime = strtok(&buf[idx], " ");
				int time = 0;
				int r = sscanf(sTime, "%d", &time);
				Search(30, time, true);
			}
			// else search with time control
			// Sök i 1/50 av kvarstående tid i middle game (efter book opening)
			// I end game?
			else {
				Search(30, 15000, true);
			}
		}
		else if (streq(buf, "stop\n")) {
			Stopped = true;
		}
		else if (streq(buf, "i\n")) {
			EnterInteractiveMode();
		}
		fgets(buf, 1024, stdin);
	}
}

int EnterInteractiveMode() {
	char scan = 0;
	while (scan != 'q')
	{
		system("@cls||clear");
		PrintGame();
		printf("m: make move\n");
		printf("c: computer move\n");
		printf("t: run tests\n");
		printf("q: quit\n");
		scanf_s(" %c", &scan, 1);
		switch (scan)
		{
		case 'm':
			manualMove();
			break;
		case 'c':
			computerMove();
			break;
		case 't':

			runAllTests();
			break;
		case 'q':
			break;
		default:
			break;
		}
	}

	return 0;
}

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	mainGame.Squares[rank * 8 + file] = type | color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
		mainGame.Squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	mainGame.KingSquares[0] = 4;
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
	mainGame.KingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	mainGame.Side = WHITE;

	mainGame.State = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
	mainGame.Material[0] = 0;
	mainGame.Material[1] = 0;
	mainGame.PositionHistoryLength = 0;
	InitScores();
	InitHash();
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

void PrintGame() {
	printf("  ---------------------------------\n");

	for (int r = 8 - 1; r >= 0; r--)
	{
		printf("%d ", r + 1);
		for (int f = 0; f < 8; f++)
		{
			PieceType piece = mainGame.Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
}

void KingPositionScore(Move move, Game* game) {
	//aproximation that endgame starts att 1900.
	int endGame = game->Material[1] - game->Material[0] < 1900 ? 1 : 0;
	game->PositionScore += KingPositionValueMatrix[endGame][game->Side >> 4][move.To];
	game->PositionScore -= KingPositionValueMatrix[endGame][game->Side >> 4][move.From];
}

void MakeMove(Move move, Game* game) {
	char f = move.From;
	char t = move.To;

	PieceType captType = game->Squares[t];
	int captColor = captType >> 4;
	int side01 = game->Side >> 4;

	//Capturing
	game->Material[captColor] -= MaterialMatrix[captColor][captType & 7];

	//removing piece from square removes its position score
	game->PositionScore -= PositionValueMatrix[captType & 7][captColor][t];

	PieceType pieceType = game->Squares[f];

	char pt = pieceType & 7;
	game->PositionScore -= PositionValueMatrix[pt][side01][f];
	game->PositionScore += PositionValueMatrix[pt][side01][t];

	game->Squares[t] = game->Squares[f];
	game->Squares[f] = NOPIECE;

	unsigned long long hash = ZobritsPieceTypesSquares[pieceType][f];
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
		game->PositionScore += PositionValueMatrix[QUEEN][side01][t];
		hash ^= ZobritsPieceTypesSquares[QUEEN | game->Side][t];

		break;
	case PromotionRook:
		game->Squares[t] = ROOK | game->Side;
		game->Material[side01] += MaterialMatrix[side01][ROOK + 6];
		game->PositionScore += PositionValueMatrix[ROOK][side01][t];
		hash ^= ZobritsPieceTypesSquares[ROOK | game->Side][t];

		break;
	case PromotionBishop:
		game->Squares[t] = BISHOP | game->Side;
		game->Material[side01] += MaterialMatrix[side01][BISHOP + 6];
		game->PositionScore += PositionValueMatrix[BISHOP][side01][t];
		hash ^= ZobritsPieceTypesSquares[BISHOP | game->Side][t];

		break;
	case PromotionKnight:
		game->Squares[move.To] = KNIGHT | game->Side;
		game->Material[side01] += MaterialMatrix[side01][KNIGHT + 6];
		game->PositionScore += PositionValueMatrix[KNIGHT][side01][t];
		hash ^= ZobritsPieceTypesSquares[KNIGHT | game->Side][t];

		break;
	case KingMove:
		game->KingSquares[side01] = t;
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.

		KingPositionScore(move, game);
		break;
	case RookMove:
		switch (move.From)
		{
		case 0:
			game->State &= ~WhiteCanCastleLong;
			hash ^= ZobritsCastlingRights[0];
			break;
		case 7:
			game->State &= ~WhiteCanCastleShort;
			hash ^= ZobritsCastlingRights[1];
			break;
		case 56:
			game->State &= ~BlackCanCastleLong;
			hash ^= ZobritsCastlingRights[2];
			break;
		case 63:
			game->State &= ~BlackCanCastleShort;
			hash ^= ZobritsCastlingRights[3];
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

		game->PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game->PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
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

		game->PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game->PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
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
		game->Material[side01] += MaterialMatrix[side01][PAWN];
		game->PositionScore -= PositionValueMatrix[PAWN][captColor][behind];
		hash ^= ZobritsPieceTypesSquares[PAWN | captColor][behind];
	}
	break;
	default:
		break;
	}
	hash ^= ZobritsSides[side01];
	game->Hash ^= hash;
	game->Side ^= 24;
	game->PositionHistory[game->PositionHistoryLength++] = game->Hash;

}

void UnMakeMove(Move move, PieceType capture, GameState prevGameState, short prevPositionScore, Game* game, unsigned long long prevHash) {

	game->Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];

	game->Squares[move.From] = game->Squares[move.To];
	game->Squares[move.To] = capture;
	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;
	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][QUEEN + 6];
		game->Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionRook:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][ROOK + 6];
		game->Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionBishop:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][BISHOP + 6];
		game->Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionKnight:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][KNIGHT + 6];
		game->Squares[move.From] = PAWN | otherSide;
		break;
	case KingMove:
		game->KingSquares[otherSide01] = move.From;
		break;
	case CastleShort:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[5 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[7 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case CastleLong:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[3 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[0 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case EnPassantCapture:
		game->Squares[move.To + Behind[otherSide01]] = PAWN | game->Side;
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][PAWN];
		break;
	default:
		break;
	}
	game->State = prevGameState;
	game->PositionScore = prevPositionScore;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->PositionHistoryLength--;
}

bool SquareAttacked(int square, char attackedBy, Game* game) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		PieceType color = pieceType & (BLACK | WHITE);

		if (color != attackedBy)
			continue;
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int captPat = PawnCapturePattern[attackedBy >> 4];
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

void SortMoves(Move* moves, int moveCount, Game* game) {

	if (game->Side == WHITE)
		QuickSort(moves, 0, moveCount - 1);
	else
		QuickSortDescending(moves, 0, moveCount - 1);
}

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo, Game* game, int depth) {
	PieceType capture = game->Squares[toSquare];
	GameState prevGameState = game->State;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	short prevPosScore = game->PositionScore;
	unsigned long long prevHash = game->Hash;

	MakeMove(move, game);
	int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
	bool legal = !SquareAttacked(kingSquare, game->Side, game);
	if (legal)
	{
		move.ScoreAtDepth = GetBestScore(game, depth);
		game->MovesBuffer[game->MovesBufferLength++] = move;
	}

	UnMakeMove(move, capture, prevGameState, prevPosScore, game, prevHash);
}

void CreateMoves(Game* game, int depth) {
	game->MovesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		if (pieceType & game->Side) {
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
						CreateMove(i, toSquare, PromotionQueen, game, depth);
						CreateMove(i, toSquare, PromotionRook, game, depth);
						CreateMove(i, toSquare, PromotionBishop, game, depth);
						CreateMove(i, toSquare, PromotionKnight, game, depth);
					}
					else if (pp == 2) {
						CreateMove(i, toSquare, EnPassant, game, depth);
					}
					else {
						CreateMove(i, toSquare, PlainMove, game, depth);
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
							CreateMove(i, toSquare, PromotionQueen, game, depth);
							CreateMove(i, toSquare, PromotionRook, game, depth);
							CreateMove(i, toSquare, PromotionBishop, game, depth);
							CreateMove(i, toSquare, PromotionKnight, game, depth);
						}
						else {
							CreateMove(i, toSquare, PlainMove, game, depth);
						}
					}
					else {
						int enpFile = (game->State & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side >> 4])
								CreateMove(i, toSquare, EnPassantCapture, game, depth);
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
						CreateMove(i, toSquare, 0, game, depth);
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
						CreateMove(i, toSquare, KingMove, game, depth);
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
								CreateMove(i, 6 + castleBlackOffset, CastleShort, game, depth);
						}
					}
					if ((game->Side & WHITE && game->State & WhiteCanCastleLong) || (game->Side & BLACK && game->State & BlackCanCastleLong)) {
						if ((game->Squares[castleBlackOffset] & 7) == ROOK &&
							game->Squares[castleBlackOffset + 1] == NOPIECE &&
							game->Squares[castleBlackOffset + 2] == NOPIECE &&
							game->Squares[castleBlackOffset + 3] == NOPIECE)
						{
							if (!SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(3 + castleBlackOffset, game->Side ^ 24, game))
								CreateMove(i, 2 + castleBlackOffset, CastleLong, game, depth);
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
								CreateMove(i, toSquare, moveInfo, game, depth);
							}
							break;
						}
						else {
							CreateMove(i, toSquare, moveInfo, game, depth);
						}
					}
				}
				break;
			}
			}
		}
	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game);
}

void CreateCaptureMoves(Game* game) {
	game->MovesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		if (pieceType & game->Side) {
			PieceType pt = pieceType & 7;
			char otherSide = game->Side ^ 24;
			switch (pt)
			{
			case PAWN:
			{
				int captPat = PawnCapturePattern[game->Side >> 4];
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					//Must be a piece of opposite color.
					if (game->Squares[toSquare] & (game->Side ^ 24))
					{
						if (toSquare < 8 || toSquare > 55) {
							CreateMove(i, toSquare, PromotionQueen, game, 0);
							CreateMove(i, toSquare, PromotionRook, game, 0);
							CreateMove(i, toSquare, PromotionBishop, game, 0);
							CreateMove(i, toSquare, PromotionKnight, game, 0);
						}
						else {
							CreateMove(i, toSquare, PlainMove, game, 0);
						}
					}
					else {
						int enpFile = (game->State & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side >> 4])
								CreateMove(i, toSquare, EnPassantCapture, game, 0);
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
						CreateMove(i, toSquare, 0, game, 0);
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
						CreateMove(i, toSquare, KingMove, game, 0);
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
							CreateMove(i, toSquare, moveInfo, game, 0);
							break;
						}
					}
				}
				break;
			}
			}
		}
	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game);
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

Move parseMove(char* sMove, MoveInfo info) {
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
	mainGame.Material[0] = 0;
	mainGame.Material[1] = 0;
	mainGame.PositionScore = 0;

	for (int i = 0; i < 64; i++)
	{
		PieceType pt = mainGame.Squares[i] & 7;
		int colorSide = (mainGame.Squares[i] & (WHITE | BLACK)) >> 4;
		mainGame.Material[colorSide] += MaterialMatrix[colorSide][pt];
		mainGame.PositionScore += PositionValueMatrix[pt][colorSide][i];
	}

	//aproximation that endgame starts att 1900.
	int endGame = mainGame.Material[1] - mainGame.Material[0] < 1900 ? 1 : 0;

	mainGame.PositionScore += KingPositionValueMatrix[endGame][0][mainGame.KingSquares[0]];
	mainGame.PositionScore += KingPositionValueMatrix[endGame][1][mainGame.KingSquares[1]];
}

void InitHash() {
	mainGame.Hash = 0;
	for (int i = 0; i < 64; i++)
		mainGame.Hash ^= ZobritsPieceTypesSquares[mainGame.Squares[i]][i];
	mainGame.Hash ^= ZobritsSides[mainGame.Side >> 4];
}

void ReadFen(char* fen) {
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		mainGame.Squares[i] = NOPIECE;
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
			mainGame.Squares[rank * 8 + file] = parsePieceType(c);
			file++;
		}
	}

	index++;
	mainGame.Side = parseSide(fen[index]);
	index++;
	index++;
	mainGame.State = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': mainGame.State |= WhiteCanCastleShort;
			break;
		case 'Q': mainGame.State |= WhiteCanCastleLong;
			break;
		case 'k': mainGame.State |= BlackCanCastleShort;
			break;
		case 'q': mainGame.State |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		mainGame.State |= (enpFile + 1);
	//todo: counters	

	for (int i = 0; i < 64; i++)
	{
		if ((mainGame.Squares[i] & 7) == KING) {
			int color = (mainGame.Squares[i] >> 4);
			mainGame.KingSquares[color] = i;
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
			while (mainGame.Squares[rank * 8 + file] == NOPIECE && file < 8)
			{
				emptyCount++;
				file++;
			}

			if (emptyCount > 0) {
				fenBuffer[index++] = '0' + emptyCount;
				file--;
			}
			else {
				fenBuffer[index++] = PieceChar(mainGame.Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = mainGame.Side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (mainGame.State & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (mainGame.State & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (mainGame.State & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (mainGame.State & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';

	char noFile = 'a' - 1;
	char enPassantFile = (mainGame.State & 15) + noFile;
	if (enPassantFile == noFile)
		fenBuffer[index++] = '-';
	else
	{
		fenBuffer[index++] = enPassantFile;
		fenBuffer[index++] = mainGame.Side == WHITE ? '6' : '3';
	}
	fenBuffer[index] = '\0';
}

int ValidMoves(Move* moves) {
	CreateMoves(&mainGame, 0);
	if (mainGame.MovesBufferLength == 0)
		return 0;

	memcpy(moves, mainGame.MovesBuffer, mainGame.MovesBufferLength * sizeof(Move));
	return mainGame.MovesBufferLength;
}

int ValidMovesOnThread(Game* game, Move* moves) {
	CreateMoves(game, 0);
	if (game->MovesBufferLength == 0)
		return 0;

	memcpy(moves, game->MovesBuffer, game->MovesBufferLength * sizeof(Move));
	return game->MovesBufferLength;
}

PlayerMove MakePlayerMoveOnThread(Game* game, char* sMove) {
	Move move = parseMove(sMove, 0);
	Move moves[100];
	int length = ValidMovesOnThread(game, moves);
	PlayerMove playerMove;
	for (int i = 0; i < length; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To) {
			playerMove.Move = moves[i];
			playerMove.Capture = game->Squares[move.To];
			playerMove.PreviousGameState = game->State;
			playerMove.Invalid = false;
			playerMove.PreviousPositionScore = game->PositionScore;
			playerMove.PreviousHash = game->Hash;

			MakeMove(moves[i], game);
			return playerMove;
		}
	}
	playerMove.Invalid = true;
	return playerMove;
}

PlayerMove MakePlayerMove(char* sMove) {
	return MakePlayerMoveOnThread(&mainGame, sMove);
}

void UnMakePlayerMove(PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.Capture, playerMove.PreviousGameState, playerMove.PreviousPositionScore, &mainGame, playerMove.PreviousHash);
}

void UnMakePlayerMoveOnThread(Game* game, PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.Capture, playerMove.PreviousGameState, playerMove.PreviousPositionScore, game, playerMove.PreviousHash);
}


short TotalMaterial(Game* game) {
	return game->Material[0] + game->Material[1];
}

void AdjustPositionImportance() {
	for (int i = 1; i < 7; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			PositionValueMatrix[i][0][s] = PositionValueMatrix[i][0][s] / 2;
			PositionValueMatrix[i][1][s] = PositionValueMatrix[i][1][s] / 2;

		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = KingPositionValueMatrix[0][0][i] / 2;
		KingPositionValueMatrix[1][0][i] = KingPositionValueMatrix[1][0][i] / 2;

		KingPositionValueMatrix[0][1][i] = KingPositionValueMatrix[0][1][i] / 2;
		KingPositionValueMatrix[1][1][i] = KingPositionValueMatrix[1][1][i] / 2;
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
		KingPositionValueMatrix[0][1][i] = -KingPositionValueMatrix[0][1][i];
		KingPositionValueMatrix[1][1][i] = -KingPositionValueMatrix[1][1][i];
	}
}

short GetScore(Game* game) {
	if (game->PositionHistoryLength > 3 && game->PositionHistory[game->PositionHistoryLength - 2] == game->Hash)
		return 0; // Three fold repetition.

	// todo 50 move rule.

	return game->Material[0] + game->Material[1] + game->PositionScore;
}


short GetBestScore(Game* game, int depth) {
	// todo: Should hash db be checked first?
	// When evaluation gets more complicated it will probably improve performance. 
	//if (depth == 0)
	/*	return GetScore(game);
	bool empty = false;
	short dbScore = getScoreFromHash(game->Hash, &empty, depth);
	if (!empty)
		return dbScore;*/

	return GetScore(game);
}

short AlphaBetaQuite(short alpha, short beta, Game* game) {

	int score = GetScore(game);
	short bestVal = 0;
	CreateCaptureMoves(game);
	int moveCount = game->MovesBufferLength;
	if (moveCount == 0)
	{
		SearchedLeafs++;
		return GetScore(game);
	}
	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));
	if (game->Side == BLACK) { //maximizing
		bestVal = alpha;
		if (score >= beta)
		{
			free(localMoves);
			return beta;
		}

		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[childMove.To];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			unsigned long long prevHash = game->Hash;

			MakeMove(childMove, game);
			int childValue = AlphaBetaQuite(bestVal, beta, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game, prevHash);
			bestVal = max(bestVal, childValue);
			if (bestVal >= beta)
				break;
		}
	}
	else { //minimizing
		bestVal = beta;
		if (score <= alpha)
		{
			free(localMoves);
			return alpha;
		}

		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[childMove.To];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			unsigned long long prevHash = game->Hash;

			MakeMove(childMove, game);
			int childValue = AlphaBetaQuite(alpha, bestVal, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game, prevHash);
			bestVal = min(bestVal, childValue);
			if (bestVal <= alpha)
				break;
		}
	}
	free(localMoves);
	return bestVal;
}

short AlphaBeta(short alpha, short beta, int depth, PieceType capture, Game* game) {
	if (Stopped)
		return GetScore(game);

	if (!depth) {
		if (capture) {
			return AlphaBetaQuite(alpha, beta, game);
		}
		SearchedLeafs++;
		return GetScore(game);
	}
	short bestVal = 0;
	CreateMoves(game, depth);
	int moveCount = game->MovesBufferLength;

	if (moveCount == 0) {
		if (SquareAttacked(game->KingSquares[game->Side >> 4], game->Side ^ 24, game))
			return game->Side == WHITE ? 8000 : -8000;//mate
		else
			return 0;//stale mate
	}

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));

	if (game->Side == BLACK) { //maximizing, black
		bestVal = alpha;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[childMove.To];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			unsigned long long prevHash = game->Hash;

			MakeMove(childMove, game);
			int childValue = AlphaBeta(bestVal, beta, depth - 1, capture, game);
			if (childValue > bestVal) {
				bestVal = childValue;
				AddBestMovesEntry(&bmTables[game->ThreadIndex], prevHash, childMove.From, childMove.To);
			}
			addHashScore(game->Hash, bestVal, depth);
			UnMakeMove(childMove, capture, state, prevPosScore, game, prevHash);
			if (bestVal >= beta)
				break;
		}
	}
	else { //minimizing, white
		bestVal = beta;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[childMove.To];
			GameState state = game->State;
			short prevPosScore = game->PositionScore;
			unsigned long long prevHash = game->Hash;
			MakeMove(childMove, game);
			short childValue = AlphaBeta(alpha, bestVal, depth - 1, capture, game);
			if (childValue < bestVal) {
				bestVal = childValue;
				AddBestMovesEntry(&bmTables[game->ThreadIndex], prevHash, childMove.From, childMove.To);
			}
			addHashScore(game->Hash, bestVal, depth);
			UnMakeMove(childMove, capture, state, prevPosScore, game, prevHash);
			if (bestVal <= alpha)
				break;
		}
	}
	free(localMoves);
	return bestVal;
}

Game* CopyMainGame(int threadNo) {
	threadGames[threadNo] = mainGame;
	threadGames[threadNo].KingSquares[0] = mainGame.KingSquares[0];
	threadGames[threadNo].KingSquares[1] = mainGame.KingSquares[1];
	threadGames[threadNo].Material[0] = mainGame.Material[0];
	threadGames[threadNo].Material[1] = mainGame.Material[1];
	threadGames[threadNo].ThreadIndex = threadNo;

	memcpy(mainGame.MovesBuffer, threadGames[threadNo].MovesBuffer, mainGame.MovesBufferLength * sizeof(Move));
	memcpy(mainGame.Squares, threadGames[threadNo].Squares, 64 * sizeof(PieceType));
	memcpy(mainGame.PositionHistory, threadGames[threadNo].PositionHistory, mainGame.PositionHistoryLength * sizeof(unsigned long long));

	return &threadGames[threadNo];
}

DWORD WINAPI DoNothingThread(int* prm) {
	Sleep(50);
	ExitThread(0);
}

// Entry point for a thread that ttarts the alphabeta tree search for a given depth and a given move.
// When finished takes next root move until they are no more.
// Sets the score on the root move pointer. They are all common for all threads.

DWORD WINAPI SearchThread(ThreadParams* prm) {
	//printf("mi %d  ti %d\n", prm->moveIndex, prm->threadID);
	do
	{
		Game* game = &(threadGames[prm->threadID]);
		Move move = prm->moves[prm->moveIndex];
		prm->moves[prm->moveIndex].ThreadIndex = prm->threadID;
		PieceType capt = game->Squares[move.To];
		GameState gameState = game->State;
		int positionScore = game->PositionScore;
		unsigned long long prevHash = game->Hash;

		MakeMove(move, game);
		bool empty = FALSE;
		short score;
		/*short dbScore = getScoreFromHash(game->Hash, &empty, prm->depth);
		if (!empty)
		{
			score = dbScore;
		}
		else {*/
		int alpha = -9000; //blacks best
		int beta = 9000; //whites best

		score = AlphaBeta(alpha, beta, prm->depth, capt, game);
		
		if (!Stopped)
			(&prm->moves[prm->moveIndex])->ScoreAtDepth = score;

		UnMakeMove(move, capt, gameState, positionScore, game, prevHash);

		if ((game->Side == WHITE && score < -7900) || (game->Side == BLACK && score > 7900))
		{
			ExitThread(0);
			return 0; //a check mate is found, no need to search further.
		}
		prm->moveIndex += SEARCH_THREADS;
	} while (prm->moveIndex < prm->moveCount);
	ExitThread(0);
	return 0;
}


void SetMovesScoreAtDepth(int depth, Move* localMoves, int moveCount) {

	int moveIndex = 0;
	//starta en tråd per drag
	//starta inte fler trådar än 8 (kärnor)
	//när en tråd är klar, starta nästa
	//när alla trådar är klara returnera
	HANDLE threadHandles[SEARCH_THREADS];

	ThreadParams* tps = malloc(sizeof(ThreadParams) * SEARCH_THREADS);

	for (int i = 0; i < SEARCH_THREADS; i++)
		CopyMainGame(i);

	for (int i = 0; i < SEARCH_THREADS; i++)
	{
		if (i > moveCount - 1) //in case more threads than moves
			threadHandles[i] = CreateThread(NULL, 0, DoNothingThread, NULL, 0, NULL);
		else {
			tps->threadID = i;
			tps->depth = depth;
			tps->moves = localMoves;
			tps->moveIndex = i;
			tps->moveCount = moveCount;
			threadHandles[i] = CreateThread(NULL, 0, SearchThread, tps, 0, NULL);
			tps++;

		}
		//todo: error handling

	}
	WaitForMultipleObjects(SEARCH_THREADS, threadHandles, TRUE, INFINITE);
	SortMoves(localMoves, moveCount, &threadGames[localMoves[0].ThreadIndex]);
	PrintBestLine(localMoves[0], depth);
}
// Background thread that sets Stopped flag after specified time in ms.
DWORD WINAPI TimeLimitWatch(int* args) {
	int ms = *args;
	clock_t start = clock();
	clock_t now = clock();
	printf("TimeLimitWatch %d\n", ms);
	while (now - start < ((ms / 1000) * CLOCKS_PER_SEC))
	{
		Sleep(100);
		now = clock();
	}

	Stopped = true;
	ExitThread(0);
	return 0;
}

// Starting point of a search for best move.
// Continues until time millis is reached or depth is reached.
// When async is set the result is printed to stdout. Not returned.
int _millis;
TopSearchParams g_topSearchParams;
Move Search(int maxDepth, int  millis, bool async) {
	HANDLE timeLimitThread = 0;
	_millis = millis;
	if (millis > 0) {
		timeLimitThread = CreateThread(NULL, 0, TimeLimitWatch, &_millis, 0, NULL);
	}

	Stopped = false;
	SearchedLeafs = 0;

	for (int i = 0; i < SEARCH_THREADS; i++)
		ClearTable(&bmTables[i]);

	g_topSearchParams.MaxDepth = maxDepth;
	g_topSearchParams.Milliseconds = millis;
	HANDLE handle = CreateThread(NULL, 0, BestMoveDeepening, NULL, 0, NULL);
	if (!async)
	{
		WaitForSingleObject(handle, INFINITE);
		if (timeLimitThread != 0)
			TerminateThread(timeLimitThread, 0);
		return g_topSearchParams.BestMove;
	}
}

int PrintBestLine(Move move, int depth) {
	char buffer[1000];
	char* pv = &buffer;
	char sMove[5];
	MoveToString(move, sMove);
	strcpy(pv, sMove);
	pv += 4;
	strcpy(pv, " ");
	pv++;
	Game* game = &threadGames[move.ThreadIndex];

	PlayerMove bestPlayerMove = MakePlayerMoveOnThread(game, sMove);
	int index = 0;
	PlayerMove moves[100];
	int movesCount = 0;
	while (true)
	{
		Move bMove = GetBestMove(&bmTables[move.ThreadIndex], game->Hash);
		if (bMove.MoveInfo == NotAMove)
			break;
		char sMove[5];
		MoveToString(bMove, sMove);
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
	printf("info depth %d score cp %d nodes %d pv %s\n", depth + 1, move.ScoreAtDepth, SearchedLeafs, buffer);
	fflush(stdout);
	return 0;
}

// Starting point of one thread that evaluates best score for every 7th root move. (If there are 7 threads)
// Increasing depth until given max depth.
DWORD WINAPI  BestMoveDeepening(void* v) {
	int maxDepth = g_topSearchParams.MaxDepth;
	clock_t start = clock();
	ClearHashTable();
	CreateMoves(&mainGame, 0);
	int moveCount = mainGame.MovesBufferLength;

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, mainGame.MovesBuffer, moveCount * sizeof(Move));

	int depth = 1;
	char bestMove[5];
	int bestScore;
	do
	{
		SetMovesScoreAtDepth(depth, localMoves, moveCount);
		//SortMoves(localMoves, moveCount, &mainGame);
		if (!Stopped) { // avbrutna depths ger felaktigt resultat.
			g_topSearchParams.BestMove = localMoves[0];
			bestScore = localMoves[0].ScoreAtDepth;
			MoveToString(localMoves[0], bestMove);
			//printf("INFO depth %d - %s\n", depth, bestMove);
			//pv, bestline. hur?
			depth++;
			if ((mainGame.Side == WHITE && bestScore < -7000) || (mainGame.Side == BLACK && bestScore > 7000))
			{
				Stopped = true;
				break; //A check mate is found, no need to search further.
			}
		}
	} while (depth <= maxDepth && !Stopped);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	int nps = SearchedLeafs / secs; // todo
	short score = localMoves[0].ScoreAtDepth;

	printf("info nodes %d nps %d score cp %d depth %d\n", SearchedLeafs, nps, score, depth);
	fflush(stdout);

	printf("bestmove %s\n", bestMove);
	fflush(stdout);

	ExitThread(0);
	return 0;
}
