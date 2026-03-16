#include <stdio.h>
#include <string.h>
#include <time.h>

#include "commons.h"
#include "utils.h"
#include "testhelpers.h"

int _failedAsserts = 0;
PerftResult perftResult = {0};

typedef struct
{
    const char *Name;
    TestFunc Func;
} RegisteredTest;

static RegisteredTest g_registeredTests[256];
static int g_registeredTestsCount = 0;

void RegisterTest(const char *name, TestFunc func)
{
    if (g_registeredTestsCount >= (int)(sizeof(g_registeredTests) / sizeof(g_registeredTests[0])))
    {
        PrintRed("Too many registered tests. Increase g_registeredTests size.\n");
        return;
    }

    g_registeredTests[g_registeredTestsCount].Name = name;
    g_registeredTests[g_registeredTestsCount].Func = func;
    g_registeredTestsCount++;
}

int GetRegisteredTestsCount(void)
{
    return g_registeredTestsCount;
}

const char *GetRegisteredTestName(int index)
{
    if (index < 0 || index >= g_registeredTestsCount)
        return "(invalid test index)";
    return g_registeredTests[index].Name;
}

void RunRegisteredTest(int index)
{
    if (index < 0 || index >= g_registeredTestsCount)
        return;
    g_registeredTests[index].Func();
}

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
        char str[64];
        snprintf(str, sizeof(str), "Expected %d", expected);
        PrintRed(str);
        printf("\n");
        snprintf(str, sizeof(str), "Actual   %d", actual);
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
        char str[64];
        snprintf(str, sizeof(str), "Expected %llu", expected);
        PrintRed(str);
        printf("\n");
        snprintf(str, sizeof(str), "Actual   %llu", actual);
        PrintRed(str);
        _failedAsserts++;
    }
}

void printPerftResults(void)
{
    printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n",
           perftResult.Captures, perftResult.Castles, perftResult.CheckMates, perftResult.Checks,
           perftResult.Enpassants, perftResult.Promotions);
}

void TimedTest(int iterations, void (*func)(int))
{
    clock_t start = clock();
    (*func)(iterations);
    clock_t stop = clock();

    float secs = (float)(stop - start) / CLOCKS_PER_SEC;
    printf("\n%.2fk iterations/s\n", iterations / (1000 * secs));
}
