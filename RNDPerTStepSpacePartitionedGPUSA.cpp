#include "RNDPerTStepSpacePartitionedGPUSA.hpp"

#include "OptimizationKernels.hpp"

#include <boost/compute/container/vector.hpp>
#include <boost/compute/random/mersenne_twister_engine.hpp>
#include <boost/compute/random/uniform_int_distribution.hpp>
#include <boost/compute/random/uniform_real_distribution.hpp>
#include <boost/compute/random/threefry_engine.hpp>
#include <boost/compute/algorithm/min_element.hpp>

namespace in
{

    RNDPerTStepSpacePartitionedGPUSA::RNDPerTStepSpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
        const GlobalRoutingCells& startingGrid,
        const OptimizationRoutingData& initialSolutions,
        float initialTemperature,
        float coolingRate,
        float eps,
        size_t maxIterations,
        size_t independentSpaces) : SpacePartitionedGPUSA(globalGrid,
            startingGrid,
            initialSolutions,
            initialTemperature,
            coolingRate,
            eps,
            maxIterations,
            independentSpaces)
    {
    }

    OptimizationSolution RNDPerTStepSpacePartitionedGPUSA::optimize()
    {
        const auto solutionsSize = m_initialSolutions.legTypes.size();

        const std::string source = krnl::getGlobalRoutingFunctions() + krnl::getSARandomsPerTemperatureStep();

        compute::program program = compute::program::create_with_source(source, m_context);

        try
        {
            program.build();

            compute::kernel kernel(program, "sa_per_temperature_step_generated_randoms");

            const unsigned int iterationSize = m_maxIterations / m_spaces.size();
            const unsigned int randomsSize = iterationSize * m_spaces.size();

            m_randomValues = compute::vector<float>(randomsSize, m_context);
            m_randomIndexes = compute::vector<unsigned int>(randomsSize, m_context);

            std::cout << "RANDOMS SIZE: " << randomsSize << '\n';

            const auto timeSeed = 2027;

            const unsigned int startIndex = std::sqrt(m_spaces.size());
            const unsigned int endIndex = m_initialSolutions.legTypes.size();

            std::cout << "startIndex: " << startIndex << '\n';
            std::cout << "endIndex: " << endIndex << '\n';

            boost::compute::function<unsigned int(float)> scaleToRange = compute::make_function_from_source<unsigned int(float)>(
                "scaleToRange",
                "uint scaleToRange(float x) {return (uint)(floor(x * (" + std::to_string(endIndex - startIndex + 1) + ")) +" + std::to_string(startIndex) + ");}");

            compute::threefry_engine<> generator(m_queue, timeSeed);
            compute::uniform_real_distribution floatDist;
            float T = m_initialTemperature;

            compute::vector<float> floatIndexes(randomsSize, m_context);
            floatDist.generate(floatIndexes.begin(), floatIndexes.end(), generator, m_queue);
            compute::transform(floatIndexes.begin(), floatIndexes.end(), m_randomIndexes.begin(), scaleToRange, m_queue);

            kernel.set_arg(0, m_netStarts);
            kernel.set_arg(1, m_netEnds);
            kernel.set_arg(2, m_legTypes);
            kernel.set_arg(3, m_horizontalGrid);
            kernel.set_arg(4, m_verticalGrid);
            kernel.set_arg(5, m_penalties);
            kernel.set_arg(6, m_randomIndexes);
            kernel.set_arg(7, m_randomValues);
            kernel.set_arg(9, static_cast<unsigned int>(solutionsSize));
            kernel.set_arg(10, static_cast<unsigned int>(m_globalGrid.getCols()));
            kernel.set_arg(11, static_cast<unsigned int>(m_globalGrid.getRows()));
            kernel.set_arg(12, iterationSize);

            while (T > m_eps)
            {
                std::cout << "T: " << T << "\n";
                kernel.set_arg(8, T);
                floatDist.generate(floatIndexes.begin(), floatIndexes.end(), generator, m_queue);
                compute::transform(floatIndexes.begin(), floatIndexes.end(), m_randomIndexes.begin(), scaleToRange, m_queue);
                floatDist.generate(m_randomValues.begin(), m_randomValues.end(), generator, m_queue);
                auto event = m_queue.enqueue_1d_range_kernel(kernel, 0, m_independentSpacesSize, 0);
                T *= m_coolingRate;
   
                event.wait();
            }

            m_queue.finish();


            const auto minPenaltyIter = compute::min_element(m_penalties.begin(), m_penalties.end(), m_queue);
            const auto minPenalty = *minPenaltyIter;

            const auto deviceDoglegBegin = std::distance(m_penalties.begin(), minPenaltyIter) * solutionsSize;
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