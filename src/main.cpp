#include "CCC.hpp"
#include <iostream>

#ifdef Testing

int main()
{
    std::vector<CCC::ArgumentInformation> argInfo(7);
    argInfo[1].type = CCC::ArgumentType::INTEGER_OR_AGREGATE;
    argInfo[1].size = 8;
    argInfo[2].type = CCC::ArgumentType::FLOAT_OR_DOUBLE;
    argInfo[2].size = 8;
    argInfo[3].type = CCC::ArgumentType::INTEGER_OR_AGREGATE;
    argInfo[3].size = 4;
    argInfo[4].type = CCC::ArgumentType::FLOAT_OR_DOUBLE;
    argInfo[4].size = 4;
    argInfo[5].type = CCC::ArgumentType::INTEGER_OR_AGREGATE;
    argInfo[5].size = 4;
    argInfo[6].type = CCC::ArgumentType::FLOAT_OR_DOUBLE;
    argInfo[6].size = 4;
    CCC::ArgumentInformation retInfo;
    retInfo.size = 1024;
    retInfo.type = CCC::ArgumentType::OTHER; // Used when a void is required
    argInfo[0] = retInfo;
    auto FuncInfo = CCC::GetFunctionDataForFunc(retInfo, argInfo);
    return 0;
}
#endif