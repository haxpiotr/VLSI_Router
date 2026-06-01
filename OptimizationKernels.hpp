#pragma once

#include <string>

namespace krnl
{
    std::string getGlobalRoutingFunctions();
    std::string getSABulkRandomsWithTemparatureSteps();
    std::string getSARandomsPerTemperatureStep();
    std::string getCreateRandomSolution();
    std::string getPlaceAndCalculatePenalty();
    std::string crossoverTwoParentsMidpoint();
    std::string crossoverTwoParentsProbability();
    std::string mutateChosenIndexes();
    std::string mutateWithProbability();
    std::string updateParticles();
    std::string calculateVelocities();
    std::string calculateTransferFunction();
    std::string createCopyMask();
    std::string createUpdatedSolutions();
}