#pragma once

#include "commons.h"

typedef void (*TestFunc)(void);

extern int _failedAsserts;
extern PerftResult perftResult;

void RegisterTest(const char *name, TestFunc func);
int GetRegisteredTestsCount(void);
const char *GetRegisteredTestName(int index);
void RunRegisteredTest(int index);

void Assert(int goodResult, const char *msg);
void AssertNot(int result, const char *msg);
void AssertAreEqual(const char *s1, const char *s2, const char *msg);
void AssertAreEqualInts(int expected, int actual, const char *msg);
void AssertAreEqualLongs(U64 expected, U64 actual, const char *msg);
void printPerftResults(void);
void TimedTest(int iterations, void (*func)(int));

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
typedef void(__cdecl *TestInitFunc)(void);
#define TEST_CASE(name)                                                                  \
    static void name(void);                                                              \
    static void __cdecl register_##name(void)                                            \
    {                                                                                    \
        RegisterTest(#name, name);                                                       \
    }                                                                                    \
    __declspec(allocate(".CRT$XCU")) TestInitFunc register_ptr_##name = register_##name; \
    static void name(void)
#else
#define TEST_CASE(name)                                            \
    static void name(void);                                        \
    static void __attribute__((constructor)) register_##name(void) \
    {                                                              \
        RegisterTest(#name, name);                                 \
    }                                                              \
    static void name(void)
#endif
