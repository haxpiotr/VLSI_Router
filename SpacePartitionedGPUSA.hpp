#pragma once

#include "SpacePartitionedSA.hpp"

namespace in
{
    class SpacePartitionedGPUSA : public SpacePartitionedSA
    {
    public:
        ~SpacePartitionedGPUSA() override = default;
        SpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const OptimizationRoutingData& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpaces);
        OptimizationRoutingData optimize() override;
    };
}