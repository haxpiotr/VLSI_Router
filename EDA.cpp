#include "EDA.hpp"

#include "OptimizationKernels.hpp"

#include <boost/compute/algorithm.hpp>
#include <boost/compute/lambda.hpp>

namespace in
{

	EDA::EDA(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
		float alpha,
		float limit) : m_globalGrid{ globalGrid },
		m_startingGrid{ startingGrid },
		m_solutionData{ solutionData },
		m_generations{ generations },
		m_populationSize{ populationSize },
		m_alpha{ alpha },
		m_limit{limit}
	{
		m_device = bc::system::default_device();
		m_context = bc::context(m_device);
		m_queue = bc::command_queue(m_context, m_device);
		m_netCount = static_cast<unsigned int>(m_solutionData.legTypes.size());
        m_generator = std::make_unique<bc::threefry_engine<>>(m_queue, static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::seconds>
            (std::chrono::system_clock::now().time_since_epoch()).count()));
		initDeviceData();
	}

	void EDA::initDeviceData()
    {
        const auto solutionsSize = m_netCount;
        const unsigned int overallSize = solutionsSize * m_populationSize;

        std::cout << "Net count: " << solutionsSize << ", overallSize: " << overallSize << '\n';

        m_netStarts = bc::vector<bc::int2_>(solutionsSize, m_context);
        m_netEnds = bc::vector<bc::int2_>(solutionsSize, m_context);
        m_legTypes = bc::vector<char>(overallSize, m_context);
        m_oldLegTypes = bc::vector<char>(overallSize, m_context);
        m_penalties = bc::vector<float>(m_populationSize, m_context);
        m_probabilities = bc::vector<float>(m_populationSize, m_context);
        m_bestIndexes = bc::vector<unsigned int>(m_populationSize, m_context);
        m_bestSolution = bc::vector<char>(solutionsSize, m_context);

        bc::fill(m_bestIndexes.begin(), m_bestIndexes.end(), 0, m_queue);

        m_randomValues = bc::vector<float>(overallSize, m_context);

        bc::fill(m_randomValues.begin(), m_randomValues.end(), 0.0f, m_queue);

        {
            std::vector<bc::int2_> netStarts(solutionsSize);
            std::vector<bc::int2_> netEnds(solutionsSize);

            for (int i = 0; i < solutionsSize; ++i)
            {
                netStarts[i] = bc::int2_{ m_solutionData.globalNetStartsX[i], m_solutionData.globalNetStartsY[i] };
                netEnds[i] = bc::int2_{ m_solutionData.globalNetEndsX[i], m_solutionData.globalNetEndsY[i] };
            }

            bc::copy(netStarts.begin(), netStarts.end(), m_netStarts.begin(), m_queue);
            bc::copy(netEnds.begin(), netEnds.end(), m_netEnds.begin(), m_queue);
        }

        std::vector<GlobalRoutingCells> grids(m_populationSize, m_startingGrid);

        {
            const auto horizontalGridSize = m_startingGrid.horizontalCells.size();
            const auto verticalGridSize = m_startingGrid.verticalCells.size();
            const auto horizontalGridsSize = horizontalGridSize * m_populationSize;
            const auto verticalGridsSize = verticalGridSize * m_populationSize;

            {
                m_horizontalGrid = bc::vector<int>(horizontalGridsSize, m_context);
                m_startingHorizontalGrid = bc::vector<int>(horizontalGridsSize, m_context);

                std::vector<int> horizontalGrids(horizontalGridsSize);

                for (int i = 0; i < horizontalGridsSize; ++i)
                {
                    horizontalGrids[i] = grids[i / horizontalGridSize].horizontalCells[i % horizontalGridSize].congestion;
                }

                bc::copy(horizontalGrids.begin(), horizontalGrids.end(), m_horizontalGrid.begin(), m_queue);
                bc::copy(horizontalGrids.begin(), horizontalGrids.end(), m_startingHorizontalGrid.begin(), m_queue);
            }

            {
                m_verticalGrid = bc::vector<int>(verticalGridsSize, m_context);
                m_startingVerticalGrid = bc::vector<int>(verticalGridsSize, m_context);

                std::vector<int> verticalGrids(verticalGridsSize);

                for (int i = 0; i < verticalGridsSize; ++i)
                {
                    verticalGrids[i] = grids[i / verticalGridSize].verticalCells[i % verticalGridSize].congestion;
                }

                bc::copy(verticalGrids.begin(), verticalGrids.end(), m_verticalGrid.begin(), m_queue);
                bc::copy(verticalGrids.begin(), verticalGrids.end(), m_startingVerticalGrid.begin(), m_queue);

            }

            {
                bc::fill(m_legTypes.begin(), m_legTypes.end(), 0, m_queue);
                bc::fill(m_bestSolution.begin(), m_bestSolution.end(), 0, m_queue);
                bc::fill(m_oldLegTypes.begin(), m_oldLegTypes.end(), 0, m_queue);
                bc::fill(m_penalties.begin(), m_penalties.end(), 0.0f, m_queue);
            }
        }
	}

    void EDA::createRandomSolutions()
    {
        const std::string source = krnl::getCreateRandomSolution();

        bc::program program = bc::program::create_with_source(source, m_context);

        try
        {
            program.build();

            bc::kernel kernel(program, "create_random_solution");

            bc::uniform_real_distribution floatDist;
            floatDist.generate(m_randomValues.begin(), m_randomValues.end(), *m_generator, m_queue);

            kernel.set_arg(0, m_legTypes);
            kernel.set_arg(1, m_randomValues);
            kernel.set_arg(2, m_netCount);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();
        }
        catch (const bc::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }
    }

    void EDA::calculatePenalties()
    {
        bc::copy(m_startingHorizontalGrid.begin(), m_startingHorizontalGrid.end(), m_horizontalGrid.begin(), m_queue);
        bc::copy(m_startingVerticalGrid.begin(), m_startingVerticalGrid.end(), m_verticalGrid.begin(), m_queue);

        const std::string source = krnl::getGlobalRoutingFunctions() + krnl::getPlaceAndCalculatePenalty();

        bc::program program = bc::program::create_with_source(source, m_context);

        const auto cols = static_cast<unsigned int>(m_globalGrid.getCols());
        const auto rows = static_cast<unsigned int>(m_globalGrid.getRows());

        try
        {
            program.build();

            bc::kernel kernel(program, "place_and_calculate_penalty");

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
        catch (const bc::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }
    }

    void EDA::selectSolution()
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        m_iter = bc::min_element(m_penalties.begin(), m_penalties.end(), m_queue);
        const auto penaltyIndex = static_cast<unsigned int>(std::distance(m_penalties.begin(), m_iter));
        const auto solutionIndex = penaltyIndex * m_netCount;
        bc::copy(m_legTypes.begin() + solutionIndex, m_legTypes.begin() + solutionIndex + m_netCount, m_bestSolution.begin(), m_queue);
        std::cout << "Selection time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << " ms\n";

    }

    void EDA::updateProbabilities()
    {
        std::chrono::steady_clock::time_point startT = std::chrono::steady_clock::now();
        const float oneMinusAlpha = 1.0f - m_alpha;
        const float minP = m_limit;
        const float maxP = 1.0f - m_limit;

        auto start = bc::make_zip_iterator(boost::make_tuple(m_bestSolution.begin(), m_probabilities.begin()));
        auto end = bc::make_zip_iterator(boost::make_tuple(m_bestSolution.end(), m_probabilities.end()));

        bc::transform(start, end, m_probabilities.begin(), bc::lambda::clamp(bc::lambda::get<0>(bc::lambda::_1)*oneMinusAlpha 
            + bc::lambda::get<1>(bc::lambda::_1)*m_alpha, minP, maxP), m_queue);

        std::cout << "Update probabilities time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startT).count() << " ms\n";
    }

    void EDA::createUpdatedSolutions()
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        const std::string source = krnl::createUpdatedSolutions();

        bc::program program = bc::program::create_with_source(source, m_context);
        bc::uniform_real_distribution floatDist;
        try
        {
            program.build();

            bc::kernel kernel(program, "create_updated_solutions");

            std::chrono::steady_clock::time_point startT = std::chrono::steady_clock::now();
            floatDist.generate(m_randomValues.begin(), m_randomValues.end(), *m_generator, m_queue);

            kernel.set_arg(0, m_legTypes);
            kernel.set_arg(1, m_probabilities);
            kernel.set_arg(2, m_randomValues);
            kernel.set_arg(3, m_netCount);

            m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

            m_queue.finish();

        }
        catch (const bc::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }
    }

	OptimizationSolution EDA::optimize()
	{
        createRandomSolutions();

        for (unsigned int i = 0; i < m_generations; ++i)
        {
            calculatePenalties();
            selectSolution();
            updateProbabilities();
            createUpdatedSolutions();
        }

        std::vector<char> bestSolution(m_bestSolution.size());
        bc::copy(m_bestSolution.begin(), m_bestSolution.end(), bestSolution.begin(), m_queue);

        std::vector<DoglegType> result(bestSolution.size());

        for (size_t i = 0; i < result.size(); ++i)
        {
            result[i] = static_cast<DoglegType>(bestSolution[i]);
        }

        const auto penaltyIndex = static_cast<unsigned int>(std::distance(m_penalties.begin(), m_iter));
        
        const auto bestHorizontalGridIndexStart =
          penaltyIndex * m_globalGrid.getCols() * m_globalGrid.getRows();
        const auto bestHorizontalGridIndexEnd =
          bestHorizontalGridIndexStart + m_globalGrid.getCols() * m_globalGrid.getRows();
        const auto bestVerticalGridIndexStart =
          penaltyIndex * m_globalGrid.getCols() * m_globalGrid.getRows()
          + m_globalGrid.getCols() * m_globalGrid.getRows();
        const auto bestVerticalGridIndexEnd =
          bestVerticalGridIndexStart + m_globalGrid.getCols() * m_globalGrid.getRows();

         std::vector<int> bestHorizontalGrid(m_horizontalGrid.size());
         std::vector<int> bestVerticalGrid(m_verticalGrid.size());

         bc::copy(m_horizontalGrid.begin() + bestHorizontalGridIndexStart,
           m_horizontalGrid.begin() + bestHorizontalGridIndexEnd,
           bestHorizontalGrid.begin(),
           m_queue);

         bc::copy(m_verticalGrid.begin() + bestVerticalGridIndexStart,
           m_verticalGrid.begin() + bestVerticalGridIndexEnd,
           bestVerticalGrid.begin(),
           m_queue);

         GlobalRoutingCells resultGrid;

         for (size_t i = 0; i < bestHorizontalGrid.size(); ++i)
         {
             resultGrid.horizontalCells.push_back(GlobalRoutingCell{ bestHorizontalGrid[i], {{0, 0},{0,0 }} });
             resultGrid.verticalCells.push_back(GlobalRoutingCell{ bestVerticalGrid[i], {{0, 0},{0,0 }} });
         }

         resultGrid.penalty = *m_iter;

		return { result , resultGrid , *m_iter};
	}

}