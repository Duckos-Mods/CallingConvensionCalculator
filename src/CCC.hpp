#pragma once
#include <array>
#include <vector>
#include <span>
#include <cstdint>
#include <stdexcept>

#if !defined(__x86_64__) && !defined(_M_X64)
	#error "This library only supports x86_64"
    #error "This is due to the fact that I only need to support x86_64 MSVC"
#endif
#ifndef TO_UNDERLYING
#define TO_UNDERLYING(x) static_cast<std::underlying_type_t<decltype(x)>>(x)
#endif
#ifndef TO_ENUM
#define TO_ENUM(enumType, val) static_cast<enumType>(val)
#endif

// Remove dumb warnings i legit can not fix like padding warnings
#pragma warning(disable: 4820)
#pragma warning(disable: 5045)
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
        VOID                = 2, // Used for void types
        OTHER               = 3, 

    };
    
    struct ArgumentInformation
    {
        size_t size;
        ArgumentType type;
    };
    struct VariableData
    {
        VariableLocation location   = VariableLocation::NONE;
        CopyType copyType           = CopyType::NONE;
        uint32_t size               = (uint32_t)-1;
        uint16_t index              = (uint16_t)-1;
        bool isStructuredReturn     = false;
    };
    struct VariableProcessingData
    {
        uint32_t size   = (uint32_t)-1; // Just loops to the max
        bool isFloat    = false; 
    };

    struct FunctionData
    {
        std::vector<VariableData> argData   = {};
        VariableData returnData             = {};

    };

    namespace Internal
    {
        inline VariableLocation IndexToRegister(bool isFloat, uint32_t index)
        {
            return TO_ENUM(VariableLocation, isFloat ? TO_UNDERLYING(VariableLocation::XMM0) + index : TO_UNDERLYING(VariableLocation::RCX) + index);
        };

        inline uint32_t RemapIndex(uint32_t Index, uint32_t Max, uint32_t Min = 3)
        {
            return Max - Index + Min;
        }

        inline std::vector<VariableData> GetVariableDataForVariableSizes(
            std::span<ArgumentInformation> variableSizes,
            bool bumpRegisters = false)
            noexcept(false)
        {
            std::vector<VariableData> variableData(variableSizes.size()); // Pre allocate :3
            std::array<VariableLocation, 4> usedRegisters{ VariableLocation::NONE, VariableLocation::NONE, VariableLocation::NONE, VariableLocation::NONE };
            uint32_t globalIndex = 0;
            uint32_t loopCount = uint32_t((variableSizes.size() > 4) ? 4 : variableSizes.size());
            uint32_t bumpCount = 0;
            if (bumpRegisters)
            {
                if (variableData.size() == 0)
                    variableData.resize(1); // Make sure structure return has enough space

                bumpCount = 1;
                VariableData vd{};
                vd.size = 8;
                vd.copyType = CopyType::ADDRESS_OF_VARIABLE;
                vd.index = 0;
                vd.location = VariableLocation::RCX;
                vd.isStructuredReturn = true;
                variableData[0] = vd;
            }

            for (uint32_t i = bumpCount; i < loopCount; i++)
            {
                globalIndex = i;
                ArgumentInformation& ai = variableSizes[i];
                VariableData vd{};
                vd.size = (uint32_t)ai.size;
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

            loopCount = uint32_t(variableSizes.size()) - 4;
            uint32_t startIndex = loopCount + 3;
            constexpr uint32_t lastIndex = 3;

            for (uint32_t i = startIndex; i > lastIndex; i--)
            {
                size_t idx = Internal::RemapIndex(i, uint32_t(variableSizes.size() - 1), lastIndex + 1);
                ArgumentInformation& ai = variableSizes[i];
                VariableData vd{};
                vd.size = (uint32_t)ai.size;
                vd.location = VariableLocation::STACK;
                vd.index = (uint32_t)i;
                if (ai.type == ArgumentType::OTHER)
                    vd.copyType = CopyType::ADDRESS_OF_VARIABLE; // This handles all types larger than 8 bytes
                else
                    vd.copyType = CopyType::VALUE_OF_VARIABLE;
                variableData[idx] = vd;
            }
            return variableData;
        }


        inline bool GetBumpRequirementsForType(VariableProcessingData VI)
        {
            if (VI.size > 8)
                return true;
            else
                return false;
        }
    }

    template<typename... Args>
    constexpr size_t GetArgCount(){return sizeof...(Args);}

    template<size_t argCount, typename... Args>
    constexpr std::array<size_t, argCount> GetVariableSizesAsArray() noexcept{return {sizeof(Args)...};}

    /*
    * @brief Gets the argument information for a types
    * @param variableProcessingData The types to get the argument information for
    * @return A vector of `ArgumentInformation` that contains the size and type of the variable
    */
    inline std::vector<ArgumentInformation> GetArgumentInformationForTypes(
            std::span<VariableProcessingData> variableProcessingData
        )
    {
        std::vector<ArgumentInformation> VariableInformation;
        VariableInformation.reserve(variableProcessingData.size());
        for (auto& VA : variableProcessingData)
        {
            ArgumentInformation AI{};
            AI.size = VA.size;
            if (VA.isFloat)
                AI.type = ArgumentType::FLOAT_OR_DOUBLE;
            else if (VA.size <= 8)
                AI.type = ArgumentType::INTEGER_OR_AGREGATE;
            else
                AI.type = ArgumentType::OTHER;
            VariableInformation.emplace_back(AI);
        }
        return VariableInformation;
    }
    /*
    * @brief Gets the return info and argument info for a function
    * @brief The first argument in the `argumentInformation` span is expected to be the structured return type if its needed
    * @brief The return information is the first element in the argData vector. So be carefull to manage that
    * @brief The structured return data is always a ADDRESS_OF_VARIABLE if structured return is needed
    * @param returnInformation The return information for the function. This can be gotten from calling `GetArgumentInformationForTypes`
    * @param argumentInformation The argument information for the function. This can be gotten from calling `GetArgumentInformationForTypes`
    * @return A `FunctionData` struct that contains the return and argument information for the function
    */
    inline FunctionData GetFunctionDataForFunc(
        ArgumentInformation returnInformation,
        std::span<ArgumentInformation> argumentInformation
    )
    {
        bool bumpRequired = (returnInformation.size > 8) ? true : false;
        auto argInformation = Internal::GetVariableDataForVariableSizes(argumentInformation, bumpRequired);
        FunctionData fd;
        if (bumpRequired)
            fd.returnData = argInformation[0];
        fd.argData = std::move(argInformation);
        return fd;

    }

    
}

#pragma warning(default: 4820)
#pragma warning(default: 5045)
