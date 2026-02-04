#include "SpacePartitionedGPUSA.hpp"

namespace in
{
    SpacePartitionedGPUSA::SpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const GlobalSolutions& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpaces) : SpacePartitionedSA(globalGrid,
                                startingGrid,
                                initialSolutions,
                                initialTemperature,
                                coolingRate,
                                eps,
                                maxIterations,
                                independentSpaces){}
    GlobalSolutions SpacePartitionedGPUSA::optimize()
    {
        return {};
    }
}