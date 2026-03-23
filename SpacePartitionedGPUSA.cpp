#include "SpacePartitionedGPUSA.hpp"

#include "OptimizationKernels.hpp"

#include <boost/compute.hpp>

namespace in
{
    namespace compute = boost::compute;
    SpacePartitionedGPUSA::SpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const OptimizationRoutingData& initialSolutions,
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
                                independentSpaces)
    {
        m_device = compute::system::default_device();
        m_context = compute::context(m_device);
        m_queue = compute::command_queue(m_context, m_device);

        initDeviceData();
    }

    void SpacePartitionedGPUSA::initDeviceData()
    {
        const auto solutionsSize = m_initialSolutions.legTypes.size();

        m_netStarts = compute::vector<compute::int2_>(solutionsSize, m_context);
        m_netEnds = compute::vector<compute::int2_>(solutionsSize, m_context);
        m_legTypes = compute::vector<char>(solutionsSize, m_context);
        m_penalties = compute::vector<float>(m_independentSpacesSize, m_context);

        {
            std::vector<compute::int2_> netStarts(solutionsSize);
            std::vector<compute::int2_> netEnds(solutionsSize);

            const auto& sortedSolution = m_spaces[0];
            for (int i = 0; i < solutionsSize; ++i)
            {
                netStarts[i] = compute::int2_{ sortedSolution.globalNetStartsX[i], sortedSolution.globalNetStartsY[i] };
                netEnds[i] = compute::int2_{ sortedSolution.globalNetEndsX[i], sortedSolution.globalNetEndsY[i] };
            }

            compute::copy(netStarts.begin(), netStarts.end(), m_netStarts.begin(), m_queue);
            compute::copy(netEnds.begin(), netEnds.end(), m_netEnds.begin(), m_queue);
        }

        std::vector<GlobalRoutingCells> grids(m_independentSpacesSize, m_startingGrid);

        {
            std::vector<float> penalties(m_independentSpacesSize);

            for (size_t i = 0; i < m_independentSpacesSize; ++i)
            {
                addSolutions(grids[i], m_spaces[i]);
                penalties[i] = grids[i].penalty;
            }

            compute::copy(penalties.begin(), penalties.end(), m_penalties.begin(), m_queue);
        }

        {
            const auto horizontalGridSize = m_startingGrid.horizontalCells.size();
            const auto verticalGridSize = m_startingGrid.verticalCells.size();
            const auto horizontalGridsSize = horizontalGridSize * m_independentSpacesSize;
            const auto verticalGridsSize = verticalGridSize * m_independentSpacesSize;
            const auto doglegTypesSize = solutionsSize * m_independentSpacesSize;

            {
                m_horizontalGrid = compute::vector<int>(horizontalGridsSize, m_context);

                std::vector<int> horizontalGrids(horizontalGridsSize);

                for (int i = 0; i < horizontalGridsSize; ++i)
                {
                    horizontalGrids[i] = grids[i / horizontalGridSize].horizontalCells[i % horizontalGridSize].congestion;
                }

                compute::copy(horizontalGrids.begin(), horizontalGrids.end(), m_horizontalGrid.begin(), m_queue);
            }

            {
                m_verticalGrid = compute::vector<int>(verticalGridsSize, m_context);

                std::vector<int> verticalGrids(verticalGridsSize);

                for (int i = 0; i < verticalGridsSize; ++i)
                {
                    verticalGrids[i] = grids[i / verticalGridSize].verticalCells[i % verticalGridSize].congestion;
                }
                
                compute::copy(verticalGrids.begin(), verticalGrids.end(), m_verticalGrid.begin(), m_queue);
            }

            {
                m_legTypes = compute::vector<char>(doglegTypesSize, m_context);
                std::vector<char> doglegTypes(doglegTypesSize);
                for (int i = 0; i < doglegTypesSize; ++i)
                {
                    doglegTypes[i] = static_cast<char>(m_spaces[i / solutionsSize].legTypes[i % solutionsSize]);
                }
                
                compute::copy(doglegTypes.begin(), doglegTypes.end(), m_legTypes.begin(), m_queue);
            }
        }

    }

    OptimizationSolution SpacePartitionedGPUSA::optimize()
    {
        const auto solutionsSize = m_initialSolutions.legTypes.size();

        const std::string source = krnl::getGlobalRoutingFunctions() + krnl::getSABulkRandomsWithTemparatureSteps();

        const auto timeSeed = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        
        compute::program program = compute::program::create_with_source(source, m_context);
        try
        {
            program.build();

            compute::kernel kernel(program, "sa_bulk_generated_randoms");

            const unsigned int totalTemperatureSteps = std::ceil(std::log(static_cast<float>(m_eps) / m_initialTemperature) / std::log(m_coolingRate));
            std::vector<float> hostTemperatures(totalTemperatureSteps);
            for (size_t i = 0; i < totalTemperatureSteps; ++i)
            {
                hostTemperatures[i] = m_initialTemperature * std::pow(m_coolingRate, i);
            }

            boost::compute::vector<float> deviceTemperatures(totalTemperatureSteps, m_context);
            boost::compute::copy(hostTemperatures.begin(), hostTemperatures.end(), deviceTemperatures.begin(), m_queue);

            std::mt19937 cpuEngine(timeSeed);
            std::uniform_int_distribution intDist(static_cast<unsigned int>(m_spaces.size()), static_cast<unsigned int>(solutionsSize));
            std::uniform_real_distribution<float> floatDist(0.0f, 1.0f);

            const unsigned int iterationSize = m_maxIterations / m_spaces.size();
            const unsigned int randomsSize = iterationSize * m_spaces.size() * totalTemperatureSteps;

            std::cout << "RANDOMS SIZE: " << randomsSize << '\n';

            boost::compute::vector<boost::compute::uint_> randomIndexes(randomsSize, m_context);
            boost::compute::vector<boost::compute::float_> randomFloats(randomsSize, m_context);

            {
                std::vector<unsigned int> hostIdx(randomsSize);
                std::vector<float> hostFloats(randomsSize);
                for (size_t i = 0; i < randomsSize; ++i)
                {
                    hostIdx[i] = intDist(cpuEngine);
                    hostFloats[i] = floatDist(cpuEngine);
                }

                boost::compute::copy(hostFloats.begin(), hostFloats.end(), randomFloats.begin(), m_queue);
                boost::compute::copy(hostIdx.begin(), hostIdx.end(), randomIndexes.begin(), m_queue);
            }

            kernel.set_arg(0, m_netStarts);
            kernel.set_arg(1, m_netEnds);
            kernel.set_arg(2, m_legTypes);
            kernel.set_arg(3, m_horizontalGrid);
            kernel.set_arg(4, m_verticalGrid);
            kernel.set_arg(5, m_penalties);
            kernel.set_arg(6, randomIndexes);
            kernel.set_arg(7, randomFloats);
            kernel.set_arg(8, deviceTemperatures);
            kernel.set_arg(9, static_cast<unsigned int>(solutionsSize));
            kernel.set_arg(10, static_cast<unsigned int>(m_globalGrid.getCols()));
            kernel.set_arg(11, static_cast<unsigned int>(m_globalGrid.getRows()));
            kernel.set_arg(12, static_cast<unsigned int>(m_spaces.size()));
            kernel.set_arg(13, iterationSize);
            kernel.set_arg(14, totalTemperatureSteps);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_independentSpacesSize, 0);
            
            m_queue.finish();

            const auto minPenaltyIter = compute::min_element(m_penalties.begin(), m_penalties.end(), m_queue);
            const auto minPenalty = *minPenaltyIter;

            const auto deviceDoglegBegin= std::distance(m_penalties.begin(), minPenaltyIter) * solutionsSize;
            const auto deviceDoglegEndIndex = deviceDoglegBegin + solutionsSize;
            
            std::vector<char> hostDoglegs(deviceDoglegEndIndex - deviceDoglegBegin);
            std::vector<DoglegType> resultDoglegs(hostDoglegs.size());

            compute::copy(m_legTypes.begin() + deviceDoglegBegin, m_legTypes.begin() + deviceDoglegEndIndex, hostDoglegs.begin(), m_queue);

            for (size_t i = 0; i < resultDoglegs.size(); ++i)
            {
                resultDoglegs[i] = hostDoglegs[i] == 0 ? DoglegType::UPPER : DoglegType::LOWER;
            }

            return { resultDoglegs, minPenalty };

        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }

        return {};
    }
}