#pragma once
#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <vector>
#include <Windows.h>
#include <cstdint>
#include <memory>
#include <stdexcept>

class FunctionCopier {
public:
    FunctionCopier();
    ~FunctionCopier() = default;

    void* CopyFunction(void* Source, size_t MaxLength = 4096) const;

private:
    ZydisDecoder Decoder_;
    ZydisFormatter Formatter_;

    struct InstructionInfo
    {
        uint8_t* Address;
        size_t Length;
        ZydisDecodedInstruction Instruction;
        ZydisDecodedOperand Operands[ZYDIS_MAX_OPERAND_COUNT];
    };

    std::vector<InstructionInfo> DisassembleFunction(void* Source, size_t MaxLength) const;
    size_t CalculateFunctionSize(const std::vector<InstructionInfo>& Instructions) const;
    void* AllocateMemory(void* Address, size_t Size) const;
    void FixRelativeAddresses(uint8_t* NewBuffer, size_t CurrentOffset, const InstructionInfo& Inst, const std::vector<InstructionInfo>& Instructions) const;
};