#pragma once

#include "SpacePartitionedGPUSA.hpp"

namespace in
{

class RNDPerTStepSpacePartitionedGPUSA : public SpacePartitionedGPUSA
{
public:
    ~RNDPerTStepSpacePartitionedGPUSA() = default;
    RNDPerTStepSpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
        const GlobalRoutingCells& startingGrid,
        const OptimizationRoutingData& initialSolutions,
        float initialTemperature,
        float coolingRate,
        float eps,
        size_t maxIterations,
        size_t independentSpaces);
    OptimizationSolution optimize() override;
};

}