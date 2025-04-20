#include "FunctionCopier.hpp"

FunctionCopier::FunctionCopier()
{
#ifdef _WIN64
    ZydisDecoderInit(&Decoder_, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
    ZydisDecoderInit(&Decoder_, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif
    ZydisFormatterInit(&Formatter_, ZYDIS_FORMATTER_STYLE_INTEL);
}

void* FunctionCopier::CopyFunction(void* Source, size_t MaxLength) const
{
    auto Instructions = DisassembleFunction(Source, MaxLength);
    size_t FunctionSize = CalculateFunctionSize(Instructions);

    void* NewFunction = AllocateMemory(Source, FunctionSize);
    if (!NewFunction)
        return nullptr;

    uint8_t* NewBuffer = static_cast<uint8_t*>(NewFunction);
    size_t CurrentOffset = 0;

    for (const auto& Inst : Instructions)
    {
        memcpy(NewBuffer + CurrentOffset, Inst.Address, Inst.Length);
        FixRelativeAddresses(NewBuffer, CurrentOffset, Inst, Instructions);
        CurrentOffset += Inst.Length;
    }

    FlushInstructionCache(GetCurrentProcess(), NewFunction, FunctionSize);

    DWORD OldProtect{};
    VirtualProtect(NewFunction, FunctionSize, PAGE_EXECUTE_READ, &OldProtect);

    return NewFunction;
}

std::vector<FunctionCopier::InstructionInfo> FunctionCopier::DisassembleFunction(void* Source, size_t MaxLength) const
{
    std::vector<InstructionInfo> Instructions;
    uint8_t* Buffer = static_cast<uint8_t*>(Source);
    size_t Offset = 0;

    while (Offset < MaxLength)
    {
        InstructionInfo Info{};
        ZyanStatus Status = ZydisDecoderDecodeFull(
            &Decoder_,
            Buffer + Offset,
            MaxLength - Offset,
            &Info.Instruction,
            Info.Operands
        );

        if (!ZYAN_SUCCESS(Status))
            break;

        Info.Address = Buffer + Offset;
        Info.Length = Info.Instruction.length;
        Instructions.push_back(Info);

        Offset += Info.Length;

        if (Info.Instruction.mnemonic == ZYDIS_MNEMONIC_RET)
            break;
    }

    return Instructions;
}

size_t FunctionCopier::CalculateFunctionSize(const std::vector<InstructionInfo>& Instructions) const
{
    size_t Size = 0;
    for (const auto& Inst : Instructions)
    {
        Size += Inst.Length;
    }
    return Size;
}

void* FunctionCopier::AllocateMemory(void* Address, size_t Size) const
{
#ifdef _WIN64
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    const uintptr_t PageSize = SystemInfo.dwPageSize;

    uintptr_t StartAddress = (uintptr_t(Address) & ~(PageSize - 1));
    uintptr_t Min = min(StartAddress - 0x7FFFFF00, (uintptr_t)SystemInfo.lpMinimumApplicationAddress);
    uintptr_t Max = max(StartAddress + 0x7FFFFF00, (uintptr_t)SystemInfo.lpMaximumApplicationAddress);

    uintptr_t StartPage = (StartAddress - (StartAddress % PageSize));
    uintptr_t Page = 1;

    while (true)
    {
        uintptr_t ByteOffset = Page * PageSize;
        uintptr_t High = StartPage + ByteOffset;
        uintptr_t Low = (StartPage > ByteOffset) ? StartPage - ByteOffset : 0;

        bool StopPoint = High > Max && Low < Min;

        if (!Low)
            continue;

        if (High < Max)
        {
            void* OutAddress = VirtualAlloc((void*)High, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (OutAddress)
                return OutAddress;
        }

        if (Low > Min)
        {
            void* OutAddress = VirtualAlloc((void*)Low, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (OutAddress)
                return OutAddress;
        }

        Page += 1;

        if (StopPoint)
            break;
    }
#else
    return VirtualAlloc(nullptr, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif

    return nullptr;
}

void FunctionCopier::FixRelativeAddresses(uint8_t* NewBuffer, size_t CurrentOffset, const InstructionInfo& Inst, const std::vector<InstructionInfo>& Instructions) const
{
    if (Inst.Instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
    {
        for (int i = 0; i < Inst.Instruction.operand_count; i++)
        {
            const auto& Op = Inst.Operands[i];

            if (Op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && Op.imm.is_relative)
            {
                uint8_t* OriginalTarget = Inst.Address + Inst.Length + Op.imm.value.s;
                int64_t NewDisplacement = OriginalTarget - (NewBuffer + CurrentOffset + Inst.Length);
                uint8_t* DisplacementLocation = NewBuffer + CurrentOffset + Inst.Instruction.raw.imm->offset;

                bool IsOutsideFunction = OriginalTarget < Instructions.front().Address || OriginalTarget >= Instructions.back().Address + Instructions.back().Length;

                if (Op.size == 8 && !IsOutsideFunction)
                    continue;

                switch (Op.size)
                {
                case 8:
                    *reinterpret_cast<int8_t*>(DisplacementLocation) = static_cast<int8_t>(NewDisplacement);
                    break;
                case 16:
                    *reinterpret_cast<int16_t*>(DisplacementLocation) = static_cast<int16_t>(NewDisplacement);
                    break;
                case 32:
                    *reinterpret_cast<int32_t*>(DisplacementLocation) = static_cast<int32_t>(NewDisplacement);
                    break;
#ifdef _WIN64
                case 64:
                    *reinterpret_cast<int64_t*>(DisplacementLocation) = static_cast<int64_t>(NewDisplacement);
                    break;
#endif
                }
            }
        }
    }

    for (int i = 0; i < Inst.Instruction.operand_count; i++)
    {
        const auto& Op = Inst.Operands[i];

        if (Op.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
#ifdef _WIN64
            if (Op.mem.base == ZYDIS_REGISTER_RIP && Op.mem.index == ZYDIS_REGISTER_NONE)
            {
#else
            if (Op.mem.base == ZYDIS_REGISTER_EIP && Op.mem.index == ZYDIS_REGISTER_NONE)
            {
#endif
                uint8_t* OriginalTarget = Inst.Address + Inst.Length + Op.mem.disp.value;
                int64_t NewDisplacement = OriginalTarget - (NewBuffer + CurrentOffset + Inst.Length);
                uint8_t* DisplacementLocation = NewBuffer + CurrentOffset + Inst.Instruction.raw.disp.offset;

                *reinterpret_cast<int32_t*>(DisplacementLocation) = static_cast<int32_t>(NewDisplacement);
            }

        }
    }
}