#pragma once

#include "../commons.h"

typedef void (*RegisteredTestFunc)(void);

typedef enum
{
	TEST_SUITE_CORE,
	TEST_SUITE_SEARCH,
} TestSuite;

typedef struct
{
	const char *Name;
	RegisteredTestFunc Func;
	TestSuite Suite;
} RegisteredTest;

void RegisterTest(const char *name, RegisteredTestFunc func, TestSuite suite);
void RunRegisteredTests(TestSuite suite);

#if defined(_MSC_VER)
typedef void(__cdecl *TestRegistrationFunc)(void);
#pragma section(".CRT$XCU", read)
#define CM_TEST_REGISTRAR(testName, suiteValue) \
	static void Register_##testName(void); \
	__declspec(allocate(".CRT$XCU")) TestRegistrationFunc Register_##testName##_ptr = Register_##testName; \
	static void Register_##testName(void) { RegisterTest(#testName, testName, suiteValue); }
#elif defined(__clang__) || defined(__GNUC__)
#define CM_TEST_REGISTRAR(testName, suiteValue) \
	static void Register_##testName(void) __attribute__((constructor)); \
	static void Register_##testName(void) { RegisterTest(#testName, testName, suiteValue); }
#else
#error Unsupported compiler for auto-registered tests.
#endif

#define CM_TEST_WITH_SUITE(testName, suiteValue) \
	static void testName(void); \
	CM_TEST_REGISTRAR(testName, suiteValue) \
	static void testName(void)

#define TEST(testName) CM_TEST_WITH_SUITE(testName, TEST_SUITE_CORE)
#define SEARCH_TEST(testName) CM_TEST_WITH_SUITE(testName, TEST_SUITE_SEARCH)

extern int _failedAsserts;
extern PerftResult perftResult;

void Assert(int goodResult, const char *msg);
void AssertNot(int result, const char *msg);
void AssertAreEqual(const char *s1, const char *s2, const char *msg);
void AssertAreEqualInts(int expected, int actual, const char *msg);
void AssertAreEqualLongs(U64 expected, U64 actual, const char *msg);
void printPerftResults();
int Perft(int depth);
int PerftHashDb(int depth);
int PerftTest(char *fen, int depth);
void TimedTest(int iterations, void (*func)(int));
bool MovesContains(Move *moves, int count, Move move);
void AssertBitboardsEqual(AllPieceBitboards expected, AllPieceBitboards actual, const char *msg);
void AssertCachedBitboardsMatchGame(const char *msg);
void AssertBestMove(int depth, const char *testName, const char *fen, const char *expected);
void AssertBestMoveTimed(int ms, const char *testName, const char *fen, const char *expected);

#ifdef _DEBUG
void PerftSaveHash(int depth);
#endif