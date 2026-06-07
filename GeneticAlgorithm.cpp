#include "GeneticAlgorithm.hpp"

#include "OptimizationKernels.hpp"

#include <boost/compute/random.hpp>
#include <boost/compute/algorithm.hpp>

namespace in
{
	GeneticAlgorithm::GeneticAlgorithm(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
        const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
        float mutationRate) : 
        m_globalRoutingGrid{ globalGrid },
        m_startingGrid{ startingGrid }, 
        m_solutionData{ solutionData },
        m_generations {generations},
        m_populationSize{ populationSize },
        m_mutationRate{mutationRate}
    {
        m_device = compute::system::default_device();
        m_context = compute::context(m_device);
        m_queue = compute::command_queue(m_context, m_device);
        m_netCount = static_cast<unsigned int>(m_solutionData.legTypes.size());

        initDeviceData();
    }

    void GeneticAlgorithm::initDeviceData()
    {
        const auto solutionsSize = m_netCount;
        const unsigned int overallSize = solutionsSize * m_populationSize;

        std::cout << "Net count: " << solutionsSize << ", overallSize: " << overallSize << '\n';

        m_netStarts = compute::vector<compute::int2_>(solutionsSize, m_context);
        m_netEnds = compute::vector<compute::int2_>(solutionsSize, m_context);
        m_legTypes = compute::vector<char>(overallSize, m_context);
        m_oldLegTypes = compute::vector<char>(overallSize, m_context);
        m_penalties = compute::vector<float>(m_populationSize, m_context);
        m_bestIndexes = compute::vector<unsigned int>(m_populationSize, m_context);

        compute::fill(m_bestIndexes.begin(), m_bestIndexes.end(), 0, m_queue);

        m_randomValues = compute::vector<float>(overallSize, m_context);

        compute::fill(m_randomValues.begin(), m_randomValues.end(), 0.0f, m_queue);

        {
            std::vector<compute::int2_> netStarts(solutionsSize);
            std::vector<compute::int2_> netEnds(solutionsSize);

            for (int i = 0; i < solutionsSize; ++i)
            {
                netStarts[i] = compute::int2_{ m_solutionData.globalNetStartsX[i], m_solutionData.globalNetStartsY[i] };
                netEnds[i] = compute::int2_{ m_solutionData.globalNetEndsX[i], m_solutionData.globalNetEndsY[i] };
            }

            compute::copy(netStarts.begin(), netStarts.end(), m_netStarts.begin(), m_queue);
            compute::copy(netEnds.begin(), netEnds.end(), m_netEnds.begin(), m_queue);
        }

        std::vector<GlobalRoutingCells> grids(m_populationSize, m_startingGrid);

        {
            const auto horizontalGridSize = m_startingGrid.horizontalCells.size();
            const auto verticalGridSize = m_startingGrid.verticalCells.size();
            const auto horizontalGridsSize = horizontalGridSize * m_populationSize;
            const auto verticalGridsSize = verticalGridSize * m_populationSize;

            {
                m_horizontalGrid = compute::vector<int>(horizontalGridsSize, m_context);
                m_startingHorizontalGrid = compute::vector<int>(horizontalGridsSize, m_context);

                std::vector<int> horizontalGrids(horizontalGridsSize);

                for (int i = 0; i < horizontalGridsSize; ++i)
                {
                    horizontalGrids[i] = grids[i / horizontalGridSize].horizontalCells[i % horizontalGridSize].congestion;
                }

                compute::copy(horizontalGrids.begin(), horizontalGrids.end(), m_horizontalGrid.begin(), m_queue);
                compute::copy(horizontalGrids.begin(), horizontalGrids.end(), m_startingHorizontalGrid.begin(), m_queue);
            }

            {
                m_verticalGrid = compute::vector<int>(verticalGridsSize, m_context);
                m_startingVerticalGrid = compute::vector<int>(verticalGridsSize, m_context);

                std::vector<int> verticalGrids(verticalGridsSize);

                for (int i = 0; i < verticalGridsSize; ++i)
                {
                    verticalGrids[i] = grids[i / verticalGridSize].verticalCells[i % verticalGridSize].congestion;
                }

                compute::copy(verticalGrids.begin(), verticalGrids.end(), m_verticalGrid.begin(), m_queue);
                compute::copy(verticalGrids.begin(), verticalGrids.end(), m_startingVerticalGrid.begin(), m_queue);

            }

            {
                compute::fill(m_legTypes.begin(), m_legTypes.end(), 0, m_queue);
                compute::fill(m_oldLegTypes.begin(), m_oldLegTypes.end(), 0, m_queue);
                compute::fill(m_penalties.begin(), m_penalties.end(), 0.0f, m_queue);
            }
        }
    }

    void GeneticAlgorithm::createRandomPopulation()
    {
        const std::string source = krnl::getGlobalRoutingFunctions() + krnl::getCreateRandomSolution();

        compute::program program = compute::program::create_with_source(source, m_context);

        const auto timeSeed = 2027;

        try
        {
            program.build();

            compute::kernel kernel(program, "create_random_solution");

            compute::uniform_real_distribution floatDist;
            compute::threefry_engine generator(m_queue, timeSeed);
            floatDist.generate(m_randomValues.begin(), m_randomValues.end(), generator, m_queue);

            kernel.set_arg(0, m_legTypes);
            kernel.set_arg(1, m_randomValues);
            kernel.set_arg(2, m_netCount);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();
        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }
    }

    void GeneticAlgorithm::calculatePenalties()
    {
        compute::copy(m_startingHorizontalGrid.begin(), m_startingHorizontalGrid.end(), m_horizontalGrid.begin(), m_queue);
        compute::copy(m_startingVerticalGrid.begin(), m_startingVerticalGrid.end(), m_verticalGrid.begin(), m_queue);

        const std::string source = krnl::getGlobalRoutingFunctions() + krnl::getPlaceAndCalculatePenalty();

        compute::program program = compute::program::create_with_source(source, m_context);

        const auto cols = static_cast<unsigned int>(m_globalRoutingGrid.getCols());
        const auto rows = static_cast<unsigned int>(m_globalRoutingGrid.getRows());

        try
        {
            program.build();

            compute::kernel kernel(program, "place_and_calculate_penalty");

            kernel.set_arg(0, m_netStarts);
            kernel.set_arg(1, m_netEnds);
            kernel.set_arg(2, m_legTypes);
            kernel.set_arg(3, m_horizontalGrid);
            kernel.set_arg(4, m_verticalGrid);
            kernel.set_arg(5, m_penalties);
            kernel.set_arg(6, m_netCount);
            kernel.set_arg(7, cols);
            kernel.set_arg(8, rows);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();
        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }
    }

    void GeneticAlgorithm::findBestSolutions()
    {
        compute::iota(m_bestIndexes.begin(), m_bestIndexes.end(), 0, m_queue);
        compute::sort_by_key(m_penalties.begin(), m_penalties.end(), m_bestIndexes.begin(), m_queue);
    }

    void GeneticAlgorithm::crossover()
    {
        compute::copy(m_legTypes.begin(), m_legTypes.end(), m_oldLegTypes.begin(), m_queue);
        
        const std::string source = krnl::crossoverTwoParentsMidpoint();

        compute::program program = compute::program::create_with_source(source, m_context);

        try
        {
            program.build();

            compute::kernel kernel(program, "crossover_two_parents_midpoint");

            kernel.set_arg(0, m_oldLegTypes);
            kernel.set_arg(1, m_legTypes);
            kernel.set_arg(2, m_bestIndexes);
            kernel.set_arg(3, m_netCount/2u);
            kernel.set_arg(4, m_netCount);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();
        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }

    }

    void GeneticAlgorithm::mutate()
    {
        compute::copy(m_legTypes.begin(), m_legTypes.end(), m_oldLegTypes.begin(), m_queue);

        const std::string source = krnl::mutateWithProbability();

        compute::program program = compute::program::create_with_source(source, m_context);

        const auto timeSeed = 2027;

        compute::uniform_real_distribution floatDist;
        compute::threefry_engine generator(m_queue, timeSeed);
        floatDist.generate(m_randomValues.begin(), m_randomValues.end(), generator, m_queue);

        try
        {
            program.build();

            compute::kernel kernel(program, "mutate_with_probability");

            kernel.set_arg(0, m_legTypes);
            kernel.set_arg(1, m_randomValues);
            kernel.set_arg(2, m_mutationRate);
            kernel.set_arg(3, m_netCount);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();
        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }

    }

	OptimizationSolution GeneticAlgorithm::optimize()
	{
        createRandomPopulation();

        for (unsigned int i = 0; i < m_generations; ++i)
        {
            calculatePenalties();
            findBestSolutions();
            crossover();
            mutate();
        }

        std::vector<unsigned int> bestIndexes(m_bestIndexes.size());
        std::vector <float> penalties(m_penalties.size());

        compute::copy(m_bestIndexes.begin(), m_bestIndexes.end(), bestIndexes.begin(), m_queue);
        compute::copy(m_penalties.begin(), m_penalties.end(), penalties.begin(), m_queue);
       
        const auto solutionSize = m_solutionData.legTypes.size();
        const auto bestSolutionIndex = bestIndexes[0] * solutionSize;

        std::vector<char> optimizedSolution(solutionSize);
        std::vector<DoglegType> result(solutionSize);
        compute::copy(m_legTypes.begin() + bestSolutionIndex, m_legTypes.begin() + bestSolutionIndex + solutionSize, optimizedSolution.begin(), m_queue);
        
        for (size_t i = 0; i < solutionSize; ++i)
        {
            result[i] = static_cast<DoglegType>(optimizedSolution[i]);
        }

        const auto bestHorizontalGridIndexBegin =
          bestIndexes[0] * m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows();
        const auto bestHorizontalGridIndexEnd =
          bestHorizontalGridIndexBegin
          + m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows();

         std::vector<int> bestHorizontalGrid(
          m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows());

         compute::copy(m_horizontalGrid.begin() + bestHorizontalGridIndexBegin,
           m_horizontalGrid.begin() + bestHorizontalGridIndexEnd,
           bestHorizontalGrid.begin(),
           m_queue);

         const auto bestVerticalGridIndexBegin = 
           bestIndexes[0] * m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows()
           + m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows();
         const auto bestVerticalGridIndexEnd = 
           bestVerticalGridIndexBegin
           + m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows();

         std::vector<int> bestVerticalGrid(
           m_globalRoutingGrid.getCols() * m_globalRoutingGrid.getRows());
         compute::copy(m_verticalGrid.begin() + bestVerticalGridIndexBegin, 
           m_verticalGrid.begin() + bestVerticalGridIndexEnd,
           bestVerticalGrid.begin(),
           m_queue);

         GlobalRoutingCells bestGrid;

         for (size_t i = 0; i < bestHorizontalGrid.size(); ++i)
         {
             bestGrid.horizontalCells.push_back({ bestHorizontalGrid[i], { {0,0}, {0,0} } });
             bestGrid.verticalCells.push_back({ bestVerticalGrid[i], { { 0, 0 }, { 0, 0 } } });
         }

         bestGrid.penalty = penalties[0];

		return { result ,bestGrid, penalties[0]};
	}
}