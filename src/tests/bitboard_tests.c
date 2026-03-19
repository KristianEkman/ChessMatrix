#include <stdio.h>
#include <time.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../commons.h"
#include "../utils.h"
#include "../hashTable.h"
#include "../evaluation.h"
#include "../search.h"
#include "../moves.h"
#include "../platform.h"
#include "../bitboards.h"

TEST(PawnBitboardsTest)
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

TEST(KnightBitboardsTest)
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

TEST(BishopBitboardsTest)
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

TEST(RookBitboardsTest)
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	RookBitboards bb = GetRookBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << a1) | (1ULL << h1), bb.WhiteRooks, "white rook bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << a8) | (1ULL << h8), bb.BlackRooks, "black rook bitboard mismatch in start position");
	AssertAreEqualLongs((1ULL << a1) | (1ULL << h1) | (1ULL << a8) | (1ULL << h8), bb.AllRooks, "all rook bitboard mismatch in start position");

	ReadFen("7k/8/6r1/8/8/8/8/R3R1K1 w - - 0 1");
	bb = GetRookBitboards(&g_mainGame);
	AssertAreEqualLongs((1ULL << a1) | (1ULL << e1), bb.WhiteRooks, "white rook bitboard mismatch in sparse position");
	AssertAreEqualLongs(1ULL << g6, bb.BlackRooks, "black rook bitboard mismatch in sparse position");
	AssertAreEqualLongs((1ULL << a1) | (1ULL << e1) | (1ULL << g6), bb.AllRooks, "all rook bitboard mismatch in sparse position");
}

TEST(QueenBitboardsTest)
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

TEST(KingBitboardsTest)
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

TEST(AllPieceBitboardsTest)
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	AllPieceBitboards bb = GetAllPieceBitboards(&g_mainGame);
	AssertAreEqualLongs(0x000000000000FFFFULL, bb.WhitePieces, "white occupancy mismatch in start position");
	AssertAreEqualLongs(0xFFFF000000000000ULL, bb.BlackPieces, "black occupancy mismatch in start position");
	AssertAreEqualLongs(0xFFFF00000000FFFFULL, bb.Occupied, "occupied mismatch in start position");

	ReadFen("8/2p5/3n4/2b1q3/2k5/2N3B1/P2QR3/6K1 w - - 0 1");
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
	U64 expectedBlackKings = (1ULL << c4);

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

TEST(BitboardHelpersTest)
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

TEST(SquareAttackedBitboardTest)
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

TEST(EvaluationMaskTablesTest)
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

TEST(CachedBitboardsAfterQuietMoveTest)
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

TEST(CachedBitboardsAfterCaptureTest)
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

TEST(CachedBitboardsAfterCastlingTest)
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

TEST(CachedBitboardsAfterEnPassantTest)
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

TEST(CachedBitboardsAfterPromotionTest)
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

TEST(CachedBitboardsAfterCapturePromotionTest)
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