#include <stdio.h>
#include <time.h>
#include <string.h>

#include "main.h"
#include "fen.h"
#include "commons.h"
#include "utils.h"
#include "hashTable.h"
#include "evaluation.h"
#include "search.h"
#include "moves.h"
#include "platform.h"
#include "bitboards.h"

#ifdef _MSC_VER
#pragma region TestsHelpers
#endif

int _failedAsserts = 0;
PerftResult perftResult = {0};

void Assert(int goodResult, const char *msg)
{
	if (goodResult == 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertNot(int result, const char *msg)
{
	if (result != 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertAreEqual(const char *s1, const char *s2, const char *msg)
{
	if (strcmp(s1, s2))
	{
		PrintRed(msg);
		printf("\n");
		printf("Expected: %s\n", s1);
		printf("Actual:   %s\n", s2);
		_failedAsserts++;
	}
}

void AssertAreEqualInts(int expected, int actual, const char *msg)
{
	if (expected != actual)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %d", expected);
		PrintRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %d", actual);
		PrintRed(str);
		_failedAsserts++;
	}
}

void AssertAreEqualLongs(U64 expected, U64 actual, const char *msg)
{
	if (expected != actual)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %llu", expected);
		PrintRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %llu", actual);
		PrintRed(str);
		_failedAsserts++;
	}
}

void printPerftResults()
{
	printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n",
		   perftResult.Captures, perftResult.Castles, perftResult.CheckMates, perftResult.Checks,
		   perftResult.Enpassants, perftResult.Promotions);
}

#ifdef _MSC_VER
#pragma endregion
#endif

void HashKeyTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
	U64 hash1 = g_mainGame.Hash;
	MakePlayerMove("g1f3");
	MakePlayerMove("g8f6");
	MakePlayerMove("f3g1");
	MakePlayerMove("f6g8");
	U64 hash2 = g_mainGame.Hash;

	AssertAreEqualLongs(hash1, hash2, "Hash keys should be equal");
}

void HashTableRoundTrip()
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;
	short expected = 3000;
	Move move1 = ParseMove("b1c1", PlainMove);
	AddHashScore(hash, expected, 1, EXACT, move1);
	short score = 0;
	Move move;
	GetScoreFromHash(hash, 1, &score, &move, 3000, 0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");

	U64 hash2 = hash + 1;
	short expected2 = -4000;
	Move move2 = ParseMove("c2e2", PlainMove);
	AddHashScore(hash2, expected2, 1, EXACT, move2);

	short score2;
	GetScoreFromHash(hash2, 1, &score2, &move, 0, 0);
	AssertAreEqualInts(expected2, score2, "hash table score missmatch");

	GetScoreFromHash(hash, 1, &score, &move, 0, 0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");
}

void HashTableDepthTest()
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;

	Move move1 = ParseMove("a6c7", PlainMove);
	AddHashScore(hash, 3000, 2, EXACT, move1);
	int depth = 2;
	short score;
	Move move;
	GetScoreFromHash(hash, depth, &score, &move, 100, 200);
	AssertAreEqualInts(3000, score, "hash table score missmatch");

	Move move2 = ParseMove("c2c2", PlainMove);
	AddHashScore(hash, 4000, 1, EXACT, move2); // smaller depth
	short score2 = 0;
	GetScoreFromHash(hash, 2, &score2, &move, 30, 60);
	AssertAreEqualInts(3000, score2, "smaller depth should not replace score");

	Move move3 = ParseMove("e3g4", PlainMove);
	AddHashScore(hash, 5000, 3, EXACT, move3); // smaller depth
	GetScoreFromHash(hash, 3, &score, &move, 30, 31);
	AssertAreEqualInts(5000, score, "larger depth should replace value");
}

void HashTablePerformance(int iterations)
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = Llrand();
	short expected = MIN_SCORE;
	int depth = 1;
	short score = 0;
	Move move;

	for (int i = 0; i < iterations; i++)
	{
		expected++;
		if (expected > MAX_SCORE)
			expected = MIN_SCORE;
		hash++;
		Move move1 = ParseMove("b1b1", PlainMove);
		AddHashScore(hash, expected, 1, EXACT, move1);
		Assert(GetScoreFromHash(hash, depth, &score, &move, 100, 200), "No score returned from hash");
		AssertAreEqualInts(expected, score, "hash table score missmatch");
	}
}
int Perft(int depth)
{
	if (depth == 0)
	{
		return 1;
	}
	int nodeCount = 0;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = g_mainGame.Squares[move.To];
		if (depth == 1)
		{
			if (capture != NOPIECE)
				perftResult.Captures++;
			if (move.MoveInfo == EnPassantCapture)
				perftResult.Captures++;
			if (move.MoveInfo == CastleLong || move.MoveInfo == CastleShort)
				perftResult.Castles++;
			if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
				perftResult.Promotions++;
			if (move.MoveInfo == EnPassantCapture)
				perftResult.Enpassants++;
		}

		Undos undos = DoMove(move, &g_mainGame);
		nodeCount += Perft(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
	return nodeCount;
}

#ifdef _DEBUG

int perftSaveHashCount = 0;
int collisionCount = 0;
int hashSplits = 0;

typedef struct
{
	U64 Hash;
	char Fen[100];
} HashFen;

HashFen HashFenDb[3000000];
void PerftSaveHash(int depth)
{
	char hasHash = FALSE;
	char fen[100];
	WriteFen(fen);

	// checking all previous fens
	for (int i = 0; i < perftSaveHashCount; i++)
	{
		if (HashFenDb[i].Hash == g_mainGame.Hash)
		{
			hasHash = TRUE;

			if (strcmp(fen, HashFenDb[i].Fen))
			{ // same hash but different fen
				collisionCount++;
				printf("\ncollision %d:\n%s\n%s\nHash: %llu", collisionCount, fen, HashFenDb[i].Fen, g_mainGame.Hash);
			}
		}
		// also check that the fen does not exists with other hash
		if (strcmp(fen, HashFenDb[i].Fen) == 0)
		{
			if (HashFenDb[i].Hash != g_mainGame.Hash)
			{
				hashSplits++;
				printf("\nHash Splits: %d\nFen: %s\n", hashSplits, fen);
			}
		}
	}

	if (!hasHash)
	{
		// adding the hash and fen to last entry
		HashFenDb[perftSaveHashCount].Hash = g_mainGame.Hash;
		strcpy_s(HashFenDb[perftSaveHashCount].Fen, 100, fen);
		perftSaveHashCount++;
		if (perftSaveHashCount % 10000 == 0)
			printf("\n%d", perftSaveHashCount);
	}

	// below is regular Perft
	if (depth == 0)
		return;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		GameState prevGameState = g_mainGame.State;
		U64 prevHash = g_mainGame.Hash;

		Undos undos = DoMove(move, &g_mainGame);
		PerftSaveHash(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
}

void PerftSaveHashTest()
{
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); // quite complicated position
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

void EnpassantCollisionsTest()
{
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("rnbqkbnr/ppp2pp1/3p4/1P2p3/7p/3PP3/P1P2PPP/RNBQKBNR w KQkq - 0 1"); // quite complicated position
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

//

#endif // _DEBUG

int PerftHashDb(int depth)
{
	if (depth == 0)
		return 1;
	int nodeCount = 0;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		Undos undos = DoMove(move, &g_mainGame);
		nodeCount += PerftHashDb(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
	return nodeCount;
}

int PerftTest(char *fen, int depth)
{

	ReadFen(fen);
	// PrintGame(&mainGame);
	int perftCount = 0;
	for (int i = 0; i < 2; i++)
	{
		clock_t start = clock();
		perftResult.Captures = 0;
		perftResult.Castles = 0;
		perftResult.CheckMates = 0;
		perftResult.Checks = 0;
		perftResult.Enpassants = 0;
		perftResult.Promotions = 0;
		short startScore = TotalMaterial(&g_mainGame);
		perftCount = Perft(depth);
		AssertAreEqualInts(startScore, TotalMaterial(&g_mainGame), "Game material missmatch");
		// printPerftResults(perftResult);
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		// printf("%.2fs\n", secs);
		// printf("%d moves\n", perftCount);
		printf("\n%.2fk moves/s\n", perftCount / (1000 * secs));

		// PrintGame(&mainGame);
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen, outFen, "Start and end FEN differ");
	return perftCount;
}

void PerftHashDbTest()
{
	printf("%s\n", __func__);
	ClearHashTable();
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	PerftHashDb(4);
	PrintHashStats();
}
void FenTest()
{
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

void FenTestHalfMoves()
{
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 23";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

void FenEnppasantTest()
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	MakePlayerMove("d2d4");
	char fen[100];
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");

	PlayerMove pm2 = MakePlayerMove("d7d6");
	UnMakePlayerMove(pm2);
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");
}

void TimedTest(int iterations, void (*func)(int))
{
	clock_t start = clock();
	(*func)(iterations);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	printf("\n%.2fk iterations/s\n", iterations / (1000 * secs));
}

void PerfTestPosition2()
{
	printf("%s\n", __func__);
	char fen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0";
	PerftTest(fen, 4);
	AssertAreEqualInts(757163, perftResult.Captures, "Captures missmatch");
	AssertAreEqualInts(128013, perftResult.Castles, "Castles missmatch");
	AssertAreEqualInts(1929, perftResult.Enpassants, "En passants missmatch");
	AssertAreEqualInts(15172, perftResult.Promotions, "Promotion missmatch");
}

void PerftTestStart()
{
	printf("%s\n", __func__);
	char startFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";
	int count = PerftTest(startFen, 5);
	AssertAreEqualInts(4865609, count, "Perft Count missmatch");
}

bool MovesContains(Move *moves, int count, Move move)
{
	for (int i = 0; i < count; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To && moves[i].MoveInfo == move.MoveInfo)
		{
			return true;
		}
	}
	return false;
}

void AssertBitboardsEqual(AllPieceBitboards expected, AllPieceBitboards actual, const char *msg)
{
	AssertAreEqualLongs(expected.WhitePieces, actual.WhitePieces, msg);
	AssertAreEqualLongs(expected.BlackPieces, actual.BlackPieces, msg);
	AssertAreEqualLongs(expected.Occupied, actual.Occupied, msg);
	AssertAreEqualLongs(expected.Pawns.WhitePawns, actual.Pawns.WhitePawns, msg);
	AssertAreEqualLongs(expected.Pawns.BlackPawns, actual.Pawns.BlackPawns, msg);
	AssertAreEqualLongs(expected.Pawns.AllPawns, actual.Pawns.AllPawns, msg);
	AssertAreEqualLongs(expected.Knights.WhiteKnights, actual.Knights.WhiteKnights, msg);
	AssertAreEqualLongs(expected.Knights.BlackKnights, actual.Knights.BlackKnights, msg);
	AssertAreEqualLongs(expected.Knights.AllKnights, actual.Knights.AllKnights, msg);
	AssertAreEqualLongs(expected.Bishops.WhiteBishops, actual.Bishops.WhiteBishops, msg);
	AssertAreEqualLongs(expected.Bishops.BlackBishops, actual.Bishops.BlackBishops, msg);
	AssertAreEqualLongs(expected.Bishops.AllBishops, actual.Bishops.AllBishops, msg);
	AssertAreEqualLongs(expected.Rooks.WhiteRooks, actual.Rooks.WhiteRooks, msg);
	AssertAreEqualLongs(expected.Rooks.BlackRooks, actual.Rooks.BlackRooks, msg);
	AssertAreEqualLongs(expected.Rooks.AllRooks, actual.Rooks.AllRooks, msg);
	AssertAreEqualLongs(expected.Queens.WhiteQueens, actual.Queens.WhiteQueens, msg);
	AssertAreEqualLongs(expected.Queens.BlackQueens, actual.Queens.BlackQueens, msg);
	AssertAreEqualLongs(expected.Queens.AllQueens, actual.Queens.AllQueens, msg);
	AssertAreEqualLongs(expected.Kings.WhiteKing, actual.Kings.WhiteKing, msg);
	AssertAreEqualLongs(expected.Kings.BlackKing, actual.Kings.BlackKing, msg);
	AssertAreEqualLongs(expected.Kings.AllKings, actual.Kings.AllKings, msg);

	for (int pieceType = PAWN; pieceType <= KING; pieceType++)
	{
		for (int side = 0; side < 2; side++)
		{
			AssertAreEqualLongs(expected.Matrix[pieceType][side], actual.Matrix[pieceType][side], msg);
		}
	}
}

void AssertCachedBitboardsMatchGame(const char *msg)
{
	AssertBitboardsEqual(GetAllPieceBitboards(&g_mainGame), g_mainGame.Bitboards, msg);
}

void ValidMovesPromotionCaptureAndCastling()
{
	printf("%s\n", __func__);
	char *fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	Move moves[MAX_MOVES];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(44, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 6;
	expectedMove.MoveInfo = CastleShort;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void LongCastling()
{
	printf("%s\n", __func__);
	char *fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	Move moves[MAX_MOVES];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(48, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 2;
	expectedMove.MoveInfo = CastleLong;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void EnPassantFromFenTest()
{
	printf("%s\n", __func__);
	char *fen = "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 3";
	ReadFen(fen);
	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
	int startGameScore = TotalMaterial(&g_mainGame);
	AssertNot(MakePlayerMove("d5e6").Invalid, "Invalid move");
	AssertAreEqualInts(startGameScore - 100, TotalMaterial(&g_mainGame), "Material should decrease by 100");
}

void EnPassantAfterMove()
{
	printf("%s\n", __func__);
	char *fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - e3 0 1";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e7e5").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void BlackCastlingRightsAfterKingMove()
{
	printf("%s\n", __func__);
	char *fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e8g8", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

void WhiteCastlingRightsAfterKingMove()
{
	printf("%s\n", __func__);
	char *fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f8e8").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e1g1", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

void MoveHashMatchesRebuiltFen()
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
	PlayerMove pm = MakePlayerMove("e2e4");
	AssertNot(pm.Invalid, "Move was not valid");

	U64 hashAfterMove = g_mainGame.Hash;
	char fen[100];
	WriteFen(fen);
	ReadFen(fen);

	AssertAreEqualLongs(hashAfterMove, g_mainGame.Hash, "Move hash should match the hash rebuilt from FEN");
}

void PromotionHashMatchesRebuiltFen()
{
	printf("%s\n", __func__);
	ReadFen("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	PlayerMove pm = MakePlayerMove("b7b8q");
	AssertNot(pm.Invalid, "Move was not valid");

	U64 hashAfterMove = g_mainGame.Hash;
	char fen[100];
	WriteFen(fen);
	ReadFen(fen);

	AssertAreEqualLongs(hashAfterMove, g_mainGame.Hash, "Promotion hash should match the hash rebuilt from FEN");
}

void CastlingRightsClearedWhenRookIsCaptured()
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/q7/R3K3 b Q - 0 1");
	PlayerMove pm = MakePlayerMove("a2a1");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertNot(g_mainGame.State & WhiteCanCastleLong, "White long castling rights should be cleared after the rook on a1 is captured");
}

void MaterialBlackPawnCapture()
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AssertAreEqualInts(-450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d4e5").Invalid, "Move was not valid");
	AssertAreEqualInts(-550, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void MaterialWhiteQueenCapture()
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d5e4").Invalid, "Move was not valid");
	AssertAreEqualInts(1000, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void MaterialCaptureAndPromotion()
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	PlayerMove pm = MakePlayerMove("b7c8");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertAreEqualInts(-1010, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(pm);
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
}

void MaterialPromotion()
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("b7b8").Invalid, "Move was not valid");
	AssertAreEqualInts(-460, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void EnPassantMaterial()
{
	printf("%s\n", __func__);
	ReadFen("r3k3/3p4/8/4P3/8/8/8/4K2R b - - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d7d5").Invalid, "Move was not valid");
	PlayerMove nextMove = MakePlayerMove("e5d6");
	AssertNot(nextMove.Invalid, "Move was not valid");
	AssertAreEqualInts(-100, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(nextMove);
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void MaterialDrawWhite()
{
	printf("%s\n", __func__);
	ReadFen("2k5/3b4/8/8/5N2/4N3/2K5/8 w - - 0 1");
	short score = GetEval(&g_mainGame);
	AssertAreEqualInts(0, score, "Game should be drawn");
}

void MaterialDrawBlack()
{
	printf("%s\n", __func__);
	ReadFen("2k5/3n4/4n3/8/8/8/4B3/3K4 b - - 0 1");
	short score = GetEval(&g_mainGame);
	AssertAreEqualInts(0, score, "Game should be drawn");
}

void AssertBestMove(int depth, const char *testName, const char *fen, const char *expected)
{
	printf("\n\n****   %s   ****\n", testName);
	ReadFen(fen);
	ClearHashTable();
	g_SearchedNodes = 0;
	clock_t start = clock();
	SetSearchDefaults();
	g_topSearchParams.MaxDepth = depth;
	MoveCoordinates bestMove = Search(false);
	clock_t stop = clock();
	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	if (g_SearchedNodes == 0)
		printf("(forced move, no search needed)\n");
	else
	{
		printf("%.2fk leafs in %.2fs\n", (float)g_SearchedNodes / 1000, secs);
		printf("%.2fk leafs/s\n", g_SearchedNodes / (1000 * secs));
	}
	char sMove[6];
	CoordinatesToString(bestMove, sMove);
	AssertAreEqual(expected, sMove, "Not the expected move");
	/*printf("\nEntries:    %d", HashTableEntries);
	printf("\nMatches:    %d", HashTableMatches);
	printf("\nFull count: %d", HashTableFullCount);*/
}

void AssertBestMoveTimed(int ms, const char *testName, const char *fen, const char *expected)
{
	printf("\n\n****   %s  (timed) ****\n", testName);
	ReadFen(fen);
	ClearHashTable();
	g_SearchedNodes = 0;
	SetSearchDefaults();
	g_topSearchParams.MoveTime = ms;
	MoveCoordinates bestMove = Search(false);
	char sMove[6];
	CoordinatesToString(bestMove, sMove);
	AssertAreEqual(expected, sMove, "Not the expected move");
	/*printf("\nEntries:    %d", HashTableEntries);
	printf("\nMatches:    %d", HashTableMatches);
	printf("\nFull count: %d", HashTableFullCount);*/
}

void BestMoveTestBlackCaptureBishop()
{
	AssertBestMove(4, __func__, "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6", "d5c4");
	// AssertBestMoveTimed(10, __func__, "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6", "d5c4");
}

void TestWhiteMateIn2()
{
	char *fen = "5k2/8/2Q5/3R4/8/8/8/4K3 w - - 2 1";
	AssertBestMove(5, __func__, fen, "d5d7");
	AssertBestMoveTimed(1000, __func__, fen, "d5d7");
}

void BlackMatesIn5Deeping()
{
	char *fen = "1k2r3/pP3pp1/8/3P1B1p/5q2/N1P2b2/PP3Pp1/R5K1 b - - 0 1";
	AssertBestMoveTimed(2000, __func__, fen, "f4h4");
}

void BestMoveByWhite1()
{
	char *fen = "r1bqkb1r/ppp1pppp/2npn3/4P3/2P5/2N2NP1/PP1P1P1P/R1BQKB1R w KQkq - 1 1";
	AssertBestMove(7, __func__, fen, "d2d4"); // to realy find the advantage of this move it takes about 10 - 15 moves deep.

	// AssertBestMoveTimed(10, __func__, fen, "d2d4");
}

void BestMoveByBlack2()
{
	char *fen = "r1r5/1p6/2kpQ3/3p4/n2P4/4P3/3q1PPP/R4RK1 b - - 0 21";
	AssertBestMove(7, __func__, fen, "a4c3");
}

void BestMoveByBlack3()
{
	char *fen = "8/kp6/8/3p4/3PnQ2/4P1P1/r2q1P1P/5RK1 b - - 2 27";
	AssertBestMove(7, __func__, fen, "d2e2");
}

// void BestMoveByWhite2() {
//	char * fen = "rn1r2k1/pp3ppp/8/3q4/3N4/P3P3/4QPPP/3R1RK1 w - - 1 19";
//	//requires atlest depth 7 to be found
//	AssertBestMove(7, __func__, fen, "d4f5");
// }

void BestMoveByBlack1()
{
	char *fen = "r1bq2k1/p1p2pp1/2p2n1p/3pr3/7B/P1PBPQ2/5PPP/R4RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "g7g5");
}

void BestMoveByBlack4()
{
	char *fen = "r1b2r2/2q2pk1/2pb3p/pp2pNpn/4Pn2/P1NB2BP/1PP1QPP1/R4RK1 b - - 0 1";
	AssertBestMove(5, __func__, fen, "c8f5");
}

void BestMoveByBlack5()
{
	char *fen = "r2qk2r/1b3pp1/pb2p2p/Rp2P3/2pPB3/2P2N2/2Q2PPP/2B2RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "b7e4");
}

void BestMoveByBlack6()
{
	char *fen = "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1";
	AssertBestMove(15, __func__, fen, "b3b2");
}

void BestMoveByWhite3()
{
	char *fen = "r4rk1/p7/1p1N3p/3nPppb/3n4/3B3P/PP1B2K1/R4R2 w - - 0 1";
	AssertBestMove(5, __func__, fen, "d3c4");
}

void RookSacrificeByWhite()
{
	char *fen = "r2q2k1/p4p1p/1rp3bB/3p4/3P1Q2/RP3P2/1KP5/4R3 w - - 3 47";
	AssertBestMoveTimed(3000, __func__, fen, "e1e8");
}

void BlackMatesIn5a()
{
	char *fen = "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - -";
	AssertBestMoveTimed(500, __func__, fen, "h4f4");
}

void WhiteMatesIn5b()
{
	char *fen = "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w";
	AssertBestMove(11, __func__, fen, "e7e8");
}

void WhiteMatesIn7()
{
	char *fen = "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11";
	AssertBestMove(15, __func__, fen, "h5h7");
}

void EngineMated()
{
	char *fen = "rn3rk1/pbppq1pQ/1p2pb2/4N3/3PN3/3B4/PPP2PPP/R3K2R b KQ - 0 11";
	AssertBestMove(8, __func__, fen, "g8h7");
}

void DeepTest()
{
	printf("%s\n", __func__);
	char *fen = "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1";
	AssertBestMove(7, __func__, fen, "b1d2");
}

void MobilityRookTest()
{
	printf("%s\n", __func__);
	char *fen = "r3kbnr/ppp1pppp/2nb4/8/2P5/2N5/PP2PPPP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	g_topSearchParams.MaxDepth = 1;
	Search(true); // mobility is calculated in movegenearion and alphabeta ... incheck
	GetEval(&g_mainGame);
	// no asserts, just entrypoint for debugging.
}

void DoublePawnsTest()
{
	printf("%s\n", __func__);
	char *fen = "r3kbnr/pp2pppp/2np4/8/2P5/2N1P3/PP2P1PP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	short score = DoublePawns(20, &g_mainGame, PAWN | WHITE);
	AssertAreEqualInts(9, score, "Double pawns score missmatch");
	AssertAreEqualInts(0, DoublePawns(8, &g_mainGame, PAWN | WHITE), "Double pawns score missmatch");
}

void OpenRookFileTest()
{
	printf("%s\n", __func__);

	ReadFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(OPEN_ROOK_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Open rook file score mismatch");

	ReadFen("4k3/p7/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(OPEN_ROOK_FILE - SEMI_OPEN_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Semi-open rook file score mismatch");

	ReadFen("4k3/p7/8/8/8/8/P7/R3K3 w - - 0 1");
	AssertAreEqualInts(0, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Closed rook file score mismatch");
}

void KingExposureTest()
{
	printf("%s\n", __func__);
	char *protected = "rnbq1rk1/pppppppp/4bn2/8/8/3B1N2/PPPPPPPP/RNBQ1RK1 w - - 0 1";
	ReadFen(protected);
	AssertAreEqualInts(6, g_mainGame.KingSquares[0], "White king is not on square 1");
	AssertAreEqualInts(62, g_mainGame.KingSquares[1], "Black king is not on square 62");

	short whiteScore = KingExposed(1, &g_mainGame);
	short blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(0, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(0, blackScore, "Not the expected exposure score for black");

	char *unprotected = "rnbq1rk1/ppppp3/4b3/8/8/3B4/1PPPP3/RNBQ1RK1 w - - 0 1";
	ReadFen(unprotected);
	whiteScore = KingExposed(6, &g_mainGame);
	blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(48, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(48, blackScore, "Not the expected exposure score for black");
}

void PassedPawnTest()
{
	printf("%s\n", __func__);
	char *fen = "rnbqkbnr/3p3p/2p3p1/5p2/P7/1P5P/2PP4/RNBQKBNR w KQkq - 0 1";
	ReadFen(fen);
	short score = PassedPawn(24, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");

	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(10, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(11, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(23, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	// black
	score = PassedPawn(42, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(51, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");

	score = PassedPawn(46, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(55, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	ReadFen("rnbqkbnr/7p/8/5p2/1pP1P3/7P/8/RNBQKBNR w KQkq - 0 1");
	score = PassedPawn(25, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 25");

	score = PassedPawn(26, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 36");

	score = PassedPawn(28, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch  square 28");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch square 37");
}

void ProtectedByPawnTest()
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/2p1p3/3b4/3N4/2P1P3/8/4K3 w - - 0 1");
	AssertAreEqualInts(16, ProtectedByPawn(d4, &g_mainGame), "White piece pawn protection mismatch");
	AssertAreEqualInts(16, ProtectedByPawn(d5, &g_mainGame), "Black piece pawn protection mismatch");
}

void indexOfTest()
{
	char *s2 = "Kristian Ekman";
	Assert(IndexOf(s2, "Ekman") == 9, "index of failed");
}

void containsNotTest()
{
	char *s2 = "Kristian Ekman";
	AssertNot(Contains(s2, "annika"), "containsNotTest failed");
}

void PawnBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	PawnBitboards bb = GetPawnBitboards(&g_mainGame);
	AssertAreEqualLongs(0x000000000000FF00ULL, bb.WhitePawns, "white pawn bitboard mismatch in start position");
	AssertAreEqualLongs(0x00FF000000000000ULL, bb.BlackPawns, "black pawn bitboard mismatch in start position");
	AssertAreEqualLongs(0x00FF00000000FF00ULL, bb.AllPawns, "all pawn bitboard mismatch in start position");

	ReadFen("4k3/2p5/8/3P4/8/8/P7/4K3 w - - 0 1");
	bb = GetPawnBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << a2) | (1ULL << d5), bb.WhitePawns, "white pawn bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << c7, bb.BlackPawns, "black pawn bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << a2) | (1ULL << d5) | (1ULL << c7), bb.AllPawns, "all pawn bitboard mismatch in sparse position");
}

void KnightBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	KnightBitboards bb = GetKnightBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << b1) | (1ULL << g1), bb.WhiteKnights, "white knight bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << b8) | (1ULL << g8), bb.BlackKnights, "black knight bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << b1) | (1ULL << g1) | (1ULL << b8) | (1ULL << g8), bb.AllKnights, "all knight bitboard mismatch in start position");

	ReadFen("4k3/8/3n4/8/8/2N5/8/4K2N w - - 0 1");
	bb = GetKnightBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << c3) | (1ULL << h1), bb.WhiteKnights, "white knight bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << d6, bb.BlackKnights, "black knight bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << c3) | (1ULL << h1) | (1ULL << d6), bb.AllKnights, "all knight bitboard mismatch in sparse position");
}

void BishopBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	BishopBitboards bb = GetBishopBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << c1) | (1ULL << f1), bb.WhiteBishops, "white bishop bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << c8) | (1ULL << f8), bb.BlackBishops, "black bishop bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << c1) | (1ULL << f1) | (1ULL << c8) | (1ULL << f8), bb.AllBishops, "all bishop bitboard mismatch in start position");

	ReadFen("4k3/8/8/2b5/8/6B1/8/1B2K3 w - - 0 1");
	bb = GetBishopBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << g3) | (1ULL << b1), bb.WhiteBishops, "white bishop bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << c5, bb.BlackBishops, "black bishop bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << g3) | (1ULL << b1) | (1ULL << c5), bb.AllBishops, "all bishop bitboard mismatch in sparse position");
}

void RookBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	RookBitboards bb = GetRookBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << a1) | (1ULL << h1), bb.WhiteRooks, "white rook bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << a8) | (1ULL << h8), bb.BlackRooks, "black rook bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << a1) | (1ULL << h1) | (1ULL << a8) | (1ULL << h8), bb.AllRooks, "all rook bitboard mismatch in start position");

	ReadFen("4k3/8/6r1/8/8/8/8/R3R1K1 w - - 0 1");
	bb = GetRookBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << a1) | (1ULL << e1), bb.WhiteRooks, "white rook bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << g6, bb.BlackRooks, "black rook bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << a1) | (1ULL << e1) | (1ULL << g6), bb.AllRooks, "all rook bitboard mismatch in sparse position");
}

void QueenBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	QueenBitboards bb = GetQueenBitboards(&g_mainGame);
	AssertAreEqualLongs(1ULL << d1, bb.WhiteQueens, "white queen bitboard mismatch in start position");
	AssertAreEqualLongs(1ULL << d8, bb.BlackQueens, "black queen bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << d1) | (1ULL << d8), bb.AllQueens, "all queen bitboard mismatch in start position");

	ReadFen("4k3/8/8/4q3/8/8/8/3QK3 w - - 0 1");
	bb = GetQueenBitboards(&g_mainGame);
	AssertAreEqualLongs(1ULL << d1, bb.WhiteQueens, "white queen bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << e5, bb.BlackQueens, "black queen bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << d1) | (1ULL << e5), bb.AllQueens, "all queen bitboard mismatch in sparse position");
}

void KingBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	KingBitboards bb = GetKingBitboards(&g_mainGame);
	AssertAreEqualLongs(1ULL << e1, bb.WhiteKing, "white king bitboard mismatch in start position");
	AssertAreEqualLongs(1ULL << e8, bb.BlackKing, "black king bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << e1) | (1ULL << e8), bb.AllKings, "all king bitboard mismatch in start position");

	ReadFen("8/8/8/8/3k4/8/8/6K1 w - - 0 1");
	bb = GetKingBitboards(&g_mainGame);
	AssertAreEqualLongs(1ULL << g1, bb.WhiteKing, "white king bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << d4, bb.BlackKing, "black king bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << g1) | (1ULL << d4), bb.AllKings, "all king bitboard mismatch in sparse position");
}

void AllPieceBitboardsTest()
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	AllPieceBitboards bb = GetAllPieceBitboards(&g_mainGame);
	AssertAreEqualLongs(0x000000000000FFFFULL, bb.WhitePieces, "white occupancy mismatch in start position");
	AssertAreEqualLongs(0xFFFF000000000000ULL, bb.BlackPieces, "black occupancy mismatch in start position");
	AssertAreEqualLongs(0xFFFF00000000FFFFULL, bb.Occupied, "occupied mismatch in start position");

	ReadFen("8/2p5/3n4/2b1q3/3k4/2N3B1/P2QR3/6K1 w - - 0 1");
	bb = GetAllPieceBitboards(&g_mainGame);

	U64 expectedWhitePawns = (1ULL << a2);
	U64 expectedBlackPawns = (1ULL << c7);
	U64 expectedWhiteKnights = (1ULL << c3);
	U64 expectedBlackKnights = (1ULL << d6);
	U64 expectedWhiteBishops = (1ULL << g3);
	U64 expectedBlackBishops = (1ULL << c5);
	U64 expectedWhiteRooks = (1ULL << e2);
	U64 expectedBlackRooks = 0ULL;
	U64 expectedWhiteQueens = (1ULL << d2);
	U64 expectedBlackQueens = (1ULL << e5);
	U64 expectedWhiteKings = (1ULL << g1);
	U64 expectedBlackKings = (1ULL << d4);

	AssertAreEqualLongs(expectedWhitePawns, bb.Pawns.WhitePawns, "white pawns mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackPawns, bb.Pawns.BlackPawns, "black pawns mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhiteKnights, bb.Knights.WhiteKnights, "white knights mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackKnights, bb.Knights.BlackKnights, "black knights mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhiteBishops, bb.Bishops.WhiteBishops, "white bishops mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackBishops, bb.Bishops.BlackBishops, "black bishops mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhiteRooks, bb.Rooks.WhiteRooks, "white rooks mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackRooks, bb.Rooks.BlackRooks, "black rooks mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhiteQueens, bb.Queens.WhiteQueens, "white queens mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackQueens, bb.Queens.BlackQueens, "black queens mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhiteKings, bb.Kings.WhiteKing, "white king mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackKings, bb.Kings.BlackKing, "black king mismatch in aggregate bitboard");

	U64 expectedWhitePieces = expectedWhitePawns | expectedWhiteKnights | expectedWhiteBishops | expectedWhiteRooks | expectedWhiteQueens | expectedWhiteKings;
	U64 expectedBlackPieces = expectedBlackPawns | expectedBlackKnights | expectedBlackBishops | expectedBlackRooks | expectedBlackQueens | expectedBlackKings;
	AssertAreEqualLongs(expectedWhitePieces, bb.WhitePieces, "white occupancy mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedBlackPieces, bb.BlackPieces, "black occupancy mismatch in aggregate bitboard");
	AssertAreEqualLongs(expectedWhitePieces | expectedBlackPieces, bb.Occupied, "occupied mismatch in aggregate bitboard");
}

void BitboardHelpersTest()
{
	printf("%s\n", __func__);

	AssertAreEqualInts(0, popcount(0ULL), "popcount should return zero for empty bitboard");
	AssertAreEqualInts(3, popcount((1ULL << a1) | (1ULL << d4) | (1ULL << h8)), "popcount should return number of set bits");

	U64 bits = (1ULL << c3) | (1ULL << a1) | (1ULL << h8);
	AssertAreEqualInts(a1, pop_lsb(&bits), "pop_lsb should return the least significant bit index first");
	AssertAreEqualLongs((1ULL << c3) | (1ULL << h8), bits, "pop_lsb should clear the least significant bit");
	AssertAreEqualInts(c3, pop_lsb(&bits), "pop_lsb should keep iterating in ascending square order");
	AssertAreEqualInts(h8, pop_lsb(&bits), "pop_lsb should return the last remaining bit");
	AssertAreEqualInts(-1, pop_lsb(&bits), "pop_lsb should return -1 for empty bitboards");
}

void SquareAttackedBitboardTest()
{
	printf("%s\n", __func__);

	ReadFen("4k3/8/8/4p3/3P4/8/4K3/8 w - - 0 1");
	Assert(SquareAttacked(e5, WHITE, &g_mainGame), "white pawn attack should be detected");
	Assert(SquareAttacked(d4, BLACK, &g_mainGame), "black pawn attack should be detected");
	Assert(SquareAttacked(d3, WHITE, &g_mainGame), "white king attack should be detected");
	AssertNot(SquareAttacked(e2, BLACK, &g_mainGame), "black should not attack e2 in the pawn test position");

	ReadFen("4k3/8/8/8/3n4/8/4K3/8 w - - 0 1");
	Assert(SquareAttacked(e2, BLACK, &g_mainGame), "black knight attack should be detected");
	AssertNot(SquareAttacked(e1, BLACK, &g_mainGame), "black knight should not attack e1 in the knight test position");

	ReadFen("4k3/8/8/8/4q3/8/4P3/4K3 w - - 0 1");
	Assert(SquareAttacked(e2, BLACK, &g_mainGame), "queen rook-like attack should be detected");
	Assert(SquareAttacked(h1, BLACK, &g_mainGame), "queen diagonal attack should be detected");
	AssertNot(SquareAttacked(e1, BLACK, &g_mainGame), "queen attack should be blocked by the pawn on e2");

	ReadFen("4k3/8/8/8/8/2b5/3P4/4K3 w - - 0 1");
	Assert(SquareAttacked(d2, BLACK, &g_mainGame), "bishop attack onto the occupied target square should be detected");
	AssertNot(SquareAttacked(e1, BLACK, &g_mainGame), "bishop attack should be blocked by the pawn on d2");
}

void EvaluationMaskTablesTest()
{
	printf("%s\n", __func__);

	AssertAreEqualLongs(0x0101010101010101ULL, FileMask[0], "file A mask mismatch");
	AssertAreEqualLongs(0x8080808080808080ULL, FileMask[7], "file H mask mismatch");
	AssertAreEqualLongs(0x0202020202020202ULL, AdjacentFileMask[0], "adjacent file mask for file A mismatch");
	AssertAreEqualLongs(0x4040404040404040ULL, AdjacentFileMask[7], "adjacent file mask for file H mismatch");

	AssertAreEqualLongs((1ULL << a2) | (1ULL << b2), KingShieldMask[0][a1], "white king shield mask on a1 mismatch");
	AssertAreEqualLongs((1ULL << f7) | (1ULL << g7) | (1ULL << h7), KingShieldMask[1][g8], "black king shield mask on g8 mismatch");

	AssertAreEqualLongs((1ULL << c3) | (1ULL << e3), PawnProtectorsMask[0][d4], "white pawn protectors on d4 mismatch");
	AssertAreEqualLongs((1ULL << c5) | (1ULL << e5), PawnProtectorsMask[1][d4], "black pawn protectors on d4 mismatch");

	AssertAreEqualLongs((1ULL << c5) | (1ULL << d5) | (1ULL << e5) |
		(1ULL << c6) | (1ULL << d6) | (1ULL << e6) |
		(1ULL << c7) | (1ULL << d7) | (1ULL << e7) |
		(1ULL << c8) | (1ULL << d8) | (1ULL << e8),
		PassedPawnMask[0][d4], "white passed pawn mask on d4 mismatch");

	AssertAreEqualLongs((1ULL << c1) | (1ULL << d1) | (1ULL << e1) |
		(1ULL << c2) | (1ULL << d2) | (1ULL << e2) |
		(1ULL << c3) | (1ULL << d3) | (1ULL << e3),
		PassedPawnMask[1][d4], "black passed pawn mask on d4 mismatch");
}

void CachedBitboardsAfterQuietMoveTest()
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before quiet move");

	PlayerMove pm = MakePlayerMove("e2e4");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after quiet move");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after quiet move undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after quiet move undo");
}

void CachedBitboardsAfterCaptureTest()
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before capture");

	PlayerMove pm = MakePlayerMove("d4e5");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after capture");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after capture undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after capture undo");
}

void CachedBitboardsAfterCastlingTest()
{
	printf("%s\n", __func__);
	ReadFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before castling");

	PlayerMove pm = MakePlayerMove("e1g1");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after castling");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after castling undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after castling undo");
}

void CachedBitboardsAfterEnPassantTest()
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before en passant");

	PlayerMove pm = MakePlayerMove("d5e6");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after en passant");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after en passant undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after en passant undo");
}

void CachedBitboardsAfterPromotionTest()
{
	printf("%s\n", __func__);
	ReadFen("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before promotion");

	PlayerMove pm = MakePlayerMove("b7b8q");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after promotion");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after promotion undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after promotion undo");
}

void CachedBitboardsAfterCapturePromotionTest()
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AllPieceBitboards start = g_mainGame.Bitboards;
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards before capture promotion");

	PlayerMove pm = MakePlayerMove("b7c8q");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after capture promotion");

	UnMakePlayerMove(pm);
	AssertBitboardsEqual(start, g_mainGame.Bitboards, "cached bitboards should restore after capture promotion undo");
	AssertCachedBitboardsMatchGame("cached bitboards should match rebuilt bitboards after capture promotion undo");
}

void PinnedMoveLegalityTest()
{
	printf("%s\n", __func__);
	ReadFen("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1");

	PlayerMove illegal = MakePlayerMove("e2d2");
	Assert(illegal.Invalid, "pinned rook should not be allowed to move off the pin line");

	PlayerMove legal = MakePlayerMove("e2e8");
	AssertNot(legal.Invalid, "pinned rook should be allowed to capture the pinning rook");
	UnMakePlayerMove(legal);
}

void CheckEvasionLegalityTest()
{
	printf("%s\n", __func__);
	ReadFen("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Assert(count > 0, "checked side should still have legal evasions in the test position");

	PlayerMove illegal = MakePlayerMove("e2f2");
	Assert(illegal.Invalid, "moves that do not answer check should be rejected");

	PlayerMove legal = MakePlayerMove("e2e3");
	AssertNot(legal.Invalid, "blocking the checking rook should stay legal");
	UnMakePlayerMove(legal);
}

void _runTests()
{
	// BestMoveTest();
	printf("\nPress any key to continue.");
	PlatformGetChar();
	PlatformClearScreen();
}

void runAllTests()
{
	/*DoublePawnsTest();

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	printf("Press any key to continue.\n");
	int c = _getch();
	return;*/

	_failedAsserts = 0;

#ifdef DEBUG
	EnpassantCollisionsTest();
	PerftSaveHashTest();
#endif // _DEBUG

	HashKeyTest();
	TimedTest(50000000, HashTablePerformance);
	PerftHashDbTest();
	HashTableRoundTrip();
	HashTableDepthTest();
	PerftTestStart();
	PerfTestPosition2();
	FenTest();
	FenTestHalfMoves();
	FenEnppasantTest();
	ValidMovesPromotionCaptureAndCastling();
	LongCastling();
	EnPassantFromFenTest();
	BlackCastlingRightsAfterKingMove();
	WhiteCastlingRightsAfterKingMove();
	MoveHashMatchesRebuiltFen();
	PromotionHashMatchesRebuiltFen();
	CastlingRightsClearedWhenRookIsCaptured();
	EnPassantAfterMove();
	MaterialBlackPawnCapture();
	MaterialWhiteQueenCapture();
	MaterialPromotion();
	MaterialCaptureAndPromotion();
	EnPassantMaterial();
	MaterialDrawBlack();
	MaterialDrawWhite();
	// MobilityRookTest();
	DoublePawnsTest();
	OpenRookFileTest();
	ProtectedByPawnTest();
	PawnBitboardsTest();
	KnightBitboardsTest();
	BishopBitboardsTest();
	RookBitboardsTest();
	QueenBitboardsTest();
	KingBitboardsTest();
	AllPieceBitboardsTest();
	BitboardHelpersTest();
	SquareAttackedBitboardTest();
	EvaluationMaskTablesTest();
	CachedBitboardsAfterQuietMoveTest();
	CachedBitboardsAfterCaptureTest();
	CachedBitboardsAfterCastlingTest();
	CachedBitboardsAfterEnPassantTest();
	CachedBitboardsAfterPromotionTest();
	CachedBitboardsAfterCapturePromotionTest();
	PinnedMoveLegalityTest();
	CheckEvasionLegalityTest();
	KingExposureTest();
	// PassedPawnTest();
	/*PositionScorePawns();
	PositionScoreKnights();
	PositionScoreCastling();*/

	clock_t start = clock();

	BestMoveTestBlackCaptureBishop();
	TestWhiteMateIn2();
	BlackMatesIn5Deeping();
	BestMoveByWhite1();
	BestMoveByWhite3();
	BestMoveByBlack1();
	BestMoveByBlack4();
	BestMoveByBlack5();
	BestMoveByBlack6();
	RookSacrificeByWhite();
	BlackMatesIn5a();
	WhiteMatesIn5b();
	WhiteMatesIn7();
	EngineMated();

	clock_t end = clock();
	float secs = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time: %.2fs\n", secs);

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	else
		PrintRed("There are failed tests.\n");

	printf("Press any key to continue.\n");
	PlatformGetChar();
	PlatformClearScreen();
}