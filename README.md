# Function Copier

Copy executable functions at runtime while fixing jumps and memory addresses.

✅ **Supports x86 and x64**

## Example
```cpp
#include "Classes/FunctionCopier.hpp"
#include <iostream>

int Add(int a, int b)
{
    return a + b;
}

int main()
{
    FunctionCopier* Copier = new FunctionCopier();

    void* CopiedFunction = Copier->CopyFunction(&Add);
    int ReturnValue = reinterpret_cast<int(*)(int, int)>(CopiedFunction)(5, 3);
    printf("Result: %d\n", ReturnValue);

    delete Copier;
    return 0;
}
