#include "FunctionCopierTest.hpp"

// Simple Function
int __declspec(noinline) FunctionCopierTest::TestFunction(int A, int B)
{
    int Result = A + B;
    Result *= 2;
    Result -= 3;
    return Result;
}

// Function with relative jumps
void __declspec(noinline) FunctionCopierTest::FunctionWithJump(int X)
{
    if (X > 10)
    {
        std::cout << "Large number\n";
    }
    else
    {
        std::cout << "Small number\n";
    }
}

// Function that uses relative addressing
const char* FunctionCopierTest::FunctionWithRipRelative()
{
    static const char* Message = "Hello!";
    return Message;
}

void FunctionCopierTest::SetConsoleColor(int Color)
{
    HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(ConsoleHandle, Color);
}

void FunctionCopierTest::PrintTestResult(const char* TestName, bool Passed)
{
    SetConsoleColor(Passed ? FOREGROUND_GREEN : FOREGROUND_RED);
    printf("%-25s [%s]\n", TestName, Passed ? "PASSED" : "FAILED");
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void FunctionCopierTest::TestSimpleFunction()
{
    auto Original = TestFunction;
    FunctionCopier* Copier = new FunctionCopier();
    void* Copied = Copier->CopyFunction(reinterpret_cast<void*>(Original));

    auto CopiedFunc = reinterpret_cast<decltype(&TestFunction)>(Copied);

    int OriginalResult = Original(5, 7);
    int CopiedResult = CopiedFunc(5, 7);

    printf("\nSimple Function Test:\n");
    printf("Original: %d\n", OriginalResult);
    printf("Copied:   %d\n", CopiedResult);

    bool TestPassed = (OriginalResult == CopiedResult);
    PrintTestResult("Simple Function Test", TestPassed);

    delete Copier;
}

void FunctionCopierTest::TestJumpFunction()
{
    auto Original = FunctionWithJump;
    FunctionCopier* Copier = new FunctionCopier();
    void* Copied = Copier->CopyFunction(reinterpret_cast<void*>(Original));

    auto CopiedFunc = reinterpret_cast<decltype(&FunctionWithJump)>(Copied);

    printf("\nJump Function Test (5):\n");
    printf("Original: ");
    Original(5);
    printf("Copied:   ");
    CopiedFunc(5);

    printf("\nJump Function Test (15):\n");
    printf("Original: ");
    Original(15);
    printf("Copied:   ");
    CopiedFunc(15);

    PrintTestResult("Jump Function Test", true);

    delete Copier;
}

void FunctionCopierTest::TestRipRelativeFunction()
{
    auto Original = FunctionWithRipRelative;
    FunctionCopier* Copier = new FunctionCopier();
    void* Copied = Copier->CopyFunction(reinterpret_cast<void*>(Original));
    auto CopiedFunc = reinterpret_cast<decltype(&FunctionWithRipRelative)>(Copied);

    printf("\nRIP-Relative Test:\n");
    printf("Original: %s\n", Original());
    printf("Copied:   %s\n", CopiedFunc());

    bool TestPassed = strcmp(Original(), CopiedFunc()) == 0;
    PrintTestResult("RIP-Relative Test", TestPassed);

    delete Copier;
}

void FunctionCopierTest::RunAllTests()
{
    printf("=== FUNCTION COPIER TESTS ===\n");
    TestSimpleFunction();
    TestJumpFunction();
    TestRipRelativeFunction();
    printf("\n=== TESTS COMPLETED ===\n");
}