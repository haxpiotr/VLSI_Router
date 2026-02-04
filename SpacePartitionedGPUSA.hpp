#pragma once

#include "SpacePartitionedSA.hpp"

#include <boost/compute.hpp>

namespace in
{
    class SpacePartitionedGPUSA : public SpacePartitionedSA
    {
    public:
        ~SpacePartitionedGPUSA() override = default;
        SpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const GlobalSolutions& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpaces);
        GlobalSolutions optimize() override;
    };
}