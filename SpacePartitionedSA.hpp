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
                            const GlobalSolutions& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpacesSize);
        GlobalSolutions optimize() override;
    
    protected:
        void initializeIndependentSpaces();
    private:
        size_t m_independentSpacesSize{0};
        std::vector<GlobalSolutions> m_spaces;
    };
}