#pragma once

#include <string>

namespace krnl
{
    std::string getGlobalRoutingFunctions();
    std::string getSABulkRandomsWithTemparatureSteps();
    std::string getSARandomsPerTemperatureStep();
    std::string getCreateRandomSolution();
    std::string getPlaceAndCalculatePenalty();
}