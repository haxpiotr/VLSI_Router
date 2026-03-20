#pragma once

#include "SeqSA.hpp"

namespace in
{
    class SpacePartitionedSA : public SeqSA
    {
    public:
        ~SpacePartitionedSA() override = default;
        SpacePartitionedSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const OptimizationRoutingData& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpacesSize);
        OptimizationSolution optimize() override;
    
    protected:
        void initializeIndependentSpaces();
        size_t m_independentSpacesSize{ 0 };  
        std::vector<OptimizationRoutingData> m_spaces;

    };
}