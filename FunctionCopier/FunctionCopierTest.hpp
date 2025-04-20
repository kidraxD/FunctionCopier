#pragma once
#include "Classes/FunctionCopier.hpp"
#include <iostream>
#include <Windows.h>

class FunctionCopierTest
{
public:
    static void RunAllTests();

private:
    static int __declspec(noinline) TestFunction(int A, int B);
    static void __declspec(noinline) FunctionWithJump(int X);
    static const char* FunctionWithRipRelative();

    static void SetConsoleColor(int Color);
    static void PrintTestResult(const char* TestName, bool Passed);

    static void TestSimpleFunction();
    static void TestJumpFunction();
    static void TestRipRelativeFunction();
};