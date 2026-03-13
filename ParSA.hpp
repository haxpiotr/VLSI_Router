#pragma once

#include "SeqSA.hpp"
#include "NetSolution.hpp"

namespace in
{
    class ParSA : public SeqSA
    {
    public:
        ~ParSA() override = default;
        ParSA(const GlobalRoutingGrid& globalGrid,
                                        const GlobalRoutingCells& startingGrid,
                                        const OptimizationRoutingData& initialSolutions,
                                        float initialTemperature,
                                        float coolingRate,
                                        float eps,
                                        size_t maxIterations);
        OptimizationRoutingData optimize() override;
    };
}