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
                                        int threads,
                                        size_t maxIterations);
        OptimizationSolution optimize() override;

    private:
        int m_threads{ 0 };
    };
}