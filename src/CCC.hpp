#pragma once
#include <array>
#include <vector>
#include <span>
#include <cstdint>
#include <stdexcept>

// Check if is x86_64 if not throw error at compile time 
#if !defined(__x86_64__) && !defined(_M_X64)
	#error "This library only supports x86_64"
#endif
#ifndef TO_UNDERLYING
#define TO_UNDERLYING(x) static_cast<std::underlying_type_t<decltype(x)>>(x)
#endif
#ifndef TO_ENUM
#define TO_ENUM(enumType, val) static_cast<enumType>(val)
#endif


namespace CCC 
{

    // namespace used for the public interface of the library
    enum class VariableLocation : uint8_t
    {
        STACK = 0,
        RCX,
        RDX,
        R8,
        R9,
        XMM0,
        XMM1,
        XMM2,
        XMM3,
        NONE  = 0xFF // Used internally
    };
    enum class CopyType : uint8_t
    {
        ADDRESS_OF_VARIABLE     = 0,
        VALUE_OF_VARIABLE       = 1, // For variables under the size limit
        NONE                    = 0xFF // Used internally
    };
    enum class CallingConvention : uint8_t
    {
        CDECL       = 0,
        STDCALL     = 1,
        FASTCALL    = 2,
        THISCALL    = 3,
        VECTORCALL  = 4,
    };
    enum class ArgumentType : uint8_t
    {
        FLOAT_OR_DOUBLE     = 0, // Left most XMM0 second XMM1 third XMM2 fourth XMM3 anything after that is stack in reverse order
        INTEGER_OR_AGREGATE = 1, // Left most RCX second RDX third R8 fourth R9 anything after that is stack in reverse order
        OTHER               = 3, 

    };
    
    struct ArgumentInformation
    {
        ArgumentType type;
		size_t size;
    };
    struct VariableData
    {
        VariableLocation location   = VariableLocation::NONE;
        CopyType copyType           = CopyType::NONE;
        uint32_t size               = -1;
        uint32_t index              = -1;

    };

    namespace Internal
    {
        inline VariableLocation IndexToRegister(bool isFloat, int index)
        {
            return TO_ENUM(VariableLocation, isFloat ? TO_UNDERLYING(VariableLocation::XMM0) + index : TO_UNDERLYING(VariableLocation::RCX) + index);
        };

        inline int RemapIndex(int Index, int Max, int Min = 3)
        {
            return Max - Index + Min;
        }
    }

    template<typename... Args>
    constexpr size_t GetArgCount(){return sizeof...(Args);}

    template<size_t argCount, typename... Args>
    constexpr std::array<size_t, argCount> GetVariableSizesAsArray() noexcept{return {sizeof(Args)...};}


    inline std::vector<VariableData> GetVariableDataForVariableSizes(
        std::span<ArgumentInformation> variableSizes,
        CallingConvention callingConvention = CallingConvention::FASTCALL) 
        throw (std::invalid_argument)
    {
        switch (callingConvention)
        {
        case CCC::CallingConvention::CDECL:
        case CCC::CallingConvention::STDCALL:
        case CCC::CallingConvention::FASTCALL:
        case CCC::CallingConvention::THISCALL:
        case CCC::CallingConvention::VECTORCALL:
            break; // Catch the valid types
        default:
            throw std::invalid_argument("Invalid calling convention");
        }
        std::vector<VariableData> variableData(variableSizes.size()); // Pre allocate :3
        std::array<VariableLocation, 4> usedRegisters{VariableLocation::NONE, VariableLocation::NONE, VariableLocation::NONE, VariableLocation::NONE};
        auto IndexToRegister = [](bool isFloat, int index) -> VariableLocation {
            return TO_ENUM(VariableLocation,isFloat ? TO_UNDERLYING(VariableLocation::XMM0) + index : TO_UNDERLYING(VariableLocation::RCX) + index);
            };
        int globalIndex = 0;
        size_t loopCount = (variableSizes.size() > 4) ? 4 : variableSizes.size();
        for (int i = 0; i < loopCount; i++)
        {
            globalIndex = i;
            ArgumentInformation& ai = variableSizes[i];
            VariableData vd{};
            vd.size = ai.size;
            switch (ai.type)
            {
            case ArgumentType::OTHER:
                vd.location = Internal::IndexToRegister(false, i);
                vd.copyType = CopyType::ADDRESS_OF_VARIABLE;
                break;
            case ArgumentType::INTEGER_OR_AGREGATE:
                vd.location = Internal::IndexToRegister(false, i);
                vd.copyType = CopyType::VALUE_OF_VARIABLE;
                break;
            case ArgumentType::FLOAT_OR_DOUBLE:
                vd.location = Internal::IndexToRegister(true, i);
                vd.copyType = CopyType::VALUE_OF_VARIABLE;
                break;
            default:
                throw std::invalid_argument("Invalid Argument Type");
                break;
            }
            vd.index = i;
            variableData[i] = vd;
        }
        if (variableSizes.size() <= 4)
            return variableData;

        loopCount = variableSizes.size() - 4;
        size_t startIndex = loopCount + 3;
        constexpr size_t lastIndex = 3;

        for (size_t i = startIndex; i > lastIndex; i--)
		{
            size_t idx = Internal::RemapIndex(i, variableSizes.size() - 1, lastIndex+1);
			ArgumentInformation& ai = variableSizes[i];
			VariableData vd{};
            vd.size = ai.size;
            vd.location = VariableLocation::STACK;
            vd.index = i;
            if (ai.type == ArgumentType::OTHER)
                vd.copyType = CopyType::ADDRESS_OF_VARIABLE;
            else
                vd.copyType = CopyType::VALUE_OF_VARIABLE;
            variableData[idx] = vd;
		}




        return variableData;
    }
}