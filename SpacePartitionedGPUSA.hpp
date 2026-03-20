#pragma once

#include "SpacePartitionedSA.hpp"

#include <boost/compute/container/vector.hpp>

namespace in
{
    namespace compute = boost::compute;

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
        OptimizationSolution optimize() override;
    protected:
        void initDeviceData();
        compute::device m_device;
        compute::context m_context;
        compute::command_queue m_queue;
        compute::vector<compute::int2_> m_netStarts;
        compute::vector<compute::int2_> m_netEnds;
        compute::vector<char> m_legTypes;
        compute::vector<float> m_penalties;
        compute::vector<unsigned int> m_randomIndexes;
        compute::vector<float> m_randomValues;
        compute::vector<int> m_horizontalGrid;
        compute::vector<int> m_verticalGrid;
    };

}