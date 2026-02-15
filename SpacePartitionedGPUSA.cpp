#include "SpacePartitionedGPUSA.hpp"

#include <boost/compute.hpp>

namespace in
{
    namespace compute = boost::compute;
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
        std::cout << "Calculating manhattan lengths for all nets on GPU..." << std::endl;
        compute::device device = compute::system::default_device();
        compute::context context(device);
        compute::command_queue queue(context, device);
        compute::vector<int> manhattanLengths(m_initialSolutions.size(), context);
        std::vector<int> hostManhattanLengths(m_initialSolutions.size());
        compute::vector<compute::int2_> deviceNetStarts(m_initialSolutions.size(), context);
        compute::vector<compute::int2_> deviceNetEnds(m_initialSolutions.size(), context);
        std::vector<int> netStarts(m_initialSolutions.size() * 2);
        std::vector<int> netEnds(m_initialSolutions.size() * 2);
        
        std::generate(netStarts.begin(), netStarts.end(), [this,n = 0]() mutable 
        {
            if(n%2)
                return m_initialSolutions[n++/2].endpoints.first.x();
            else
                return m_initialSolutions[n++/2].endpoints.first.y();
        });

        std::cout << "--- GPU Manhattan lengths calculation ---" << std::endl;

        std::generate(netEnds.begin(), netEnds.end(), [this,n = 0]() mutable 
        {
            if(n%2)
                return m_initialSolutions[n++/2].endpoints.second.x();
            else
                return m_initialSolutions[n++/2].endpoints.second.y();
        });


        compute::copy(reinterpret_cast<compute::int2_*>(netStarts.data()), reinterpret_cast<compute::int2_*>(netStarts.data()) + m_initialSolutions.size(), deviceNetStarts.begin(), queue);
        compute::copy(reinterpret_cast<compute::int2_*>(netEnds.data()), reinterpret_cast<compute::int2_*>(netEnds.data()) + m_initialSolutions.size(), deviceNetEnds.begin(), queue);
        
        BOOST_COMPUTE_FUNCTION(int, manhattanLength, (compute::int2_ start, compute::int2_ end), 
        {
            return abs(start.x - end.x) + abs(start.y - end.y);
        });

        compute::transform(
            deviceNetStarts.begin(), deviceNetStarts.end(),
            deviceNetEnds.begin(),
            manhattanLengths.begin(),
            manhattanLength,
            queue
        );

        compute::sort_by_key(manhattanLengths.begin(), manhattanLengths.end(), deviceNetStarts.begin(),queue);

        compute::copy(manhattanLengths.begin(), manhattanLengths.end(), hostManhattanLengths.begin(), queue);

        return {};
    }
}