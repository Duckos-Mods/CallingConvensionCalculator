#include "CCC.hpp"
#include <iostream>

int main()
{
    std::cout << "Hello World" << std::endl;
    std::vector<CCC::ArgumentInformation> argInfo(6);
    for (int i = 0; i < 6; i++)
	{
		argInfo[i].size = 4;
		argInfo[i].type = CCC::ArgumentType::FLOAT_OR_DOUBLE;
	}
    argInfo[0].type = CCC::ArgumentType::INTEGER_OR_AGREGATE;
    argInfo[2].type = CCC::ArgumentType::INTEGER_OR_AGREGATE;
    auto VarLocations = CCC::GetVariableDataForVariableSizes(argInfo);
    return 0;
}
