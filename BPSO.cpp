#include "BPSO.hpp"

#include <boost/compute/random/uniform_real_distribution.hpp>
#include <boost/compute/algorithm.hpp>
#include <boost/compute/lambda.hpp>

#include "OptimizationKernels.hpp"

namespace in
{
	BPSO::BPSO(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
		float w,
		float c_1,
		float c_2,
		float maxV) :
		m_globalRoutingGrid{ globalGrid },
		m_startingGrid{ startingGrid },
		m_solutionData{ solutionData },
		m_generations{ generations },
		m_populationSize{ populationSize },
		m_w{ w },
		m_c1{c_1},
		m_c2{c_2},
		m_maxV{maxV}
	{
		m_device = compute::system::default_device();
		m_context = compute::context(m_device);
		m_queue = compute::command_queue(m_context, m_device);
		m_netCount = static_cast<unsigned int>(m_solutionData.legTypes.size());
		const auto timeSeed = 2027;
		m_generator = std::make_unique<compute::mt19937>(m_queue, timeSeed);

		initDeviceData();
	}
	void BPSO::initDeviceData()
	{
		const auto solutionsSize = m_netCount;
		const unsigned int overallSize = solutionsSize * m_populationSize;

		std::cout << "Net count: " << solutionsSize << ", overallSize: " << overallSize << '\n';

		m_netStarts = compute::vector<compute::int2_>(solutionsSize, m_context);
		m_netEnds = compute::vector<compute::int2_>(solutionsSize, m_context);
		m_legTypes = compute::vector<char>(overallSize, m_context);
		m_bestLegTypes = compute::vector<char>(overallSize, m_context);
		m_penalties = compute::vector<float>(m_populationSize, m_context);
		m_bestPenalties = compute::vector<float>(m_populationSize, m_context);
		m_bestIndexes = compute::vector<unsigned int>(m_populationSize, m_context);
		m_copyMask = compute::vector<char>(overallSize, m_context);
		m_velocities = compute::vector<float>(overallSize, m_context);
		m_sigmas = compute::vector<float>(overallSize, m_context);

		compute::fill(m_bestIndexes.begin(), m_bestIndexes.end(), 0, m_queue);

		m_randomValues = compute::vector<float>(overallSize, m_context);
		m_randoms1 = compute::vector<float>(overallSize, m_context);
		m_randoms2 = compute::vector<float>(overallSize, m_context);

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
				compute::fill(m_bestLegTypes.begin(), m_bestLegTypes.end(), 0, m_queue);
				compute::fill(m_penalties.begin(), m_penalties.end(), 0.0f, m_queue);
				compute::fill(m_bestPenalties.begin(), m_bestPenalties.end(), std::numeric_limits<float>::max(), m_queue);
			}
		}
	}

	void BPSO::updateBestSolutions()
	{

	}
	
	void BPSO::createRandomParticles()
	{
		compute::uniform_real_distribution velocityDistribution(-m_maxV, m_maxV);
		velocityDistribution.generate(m_velocities.begin(), m_velocities.end(), *m_generator, m_queue);
		
		compute::uniform_real_distribution randDistribution;
		randDistribution.generate(m_randomValues.begin(), m_randomValues.end(), *m_generator, m_queue);

		const std::string source = krnl::getCreateRandomSolution();

		compute::program program = compute::program::create_with_source(source, m_context);

		try
		{
			program.build();

			compute::kernel kernel(program, "create_random_solution");

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

	void BPSO::calculatePenalties()
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

			std::vector <float> penalties(m_penalties.size());

			compute::copy(m_penalties.begin(), m_penalties.end(), penalties.begin(), m_queue);

			for (size_t i = 0; i < 1; ++i)
			{
				std::cout << "i: " << i << " -> " << penalties[i] << '\n';
			}
		}
		catch (const compute::opencl_error& e)
		{
			std::cerr << "OpenCL error: " << e.what() << std::endl;
			std::cout << program.build_log() << std::endl;
		}
	}

	void BPSO::findBestSolutions()
	{
		compute::iota(m_bestIndexes.begin(), m_bestIndexes.end(), 0, m_queue);
		compute::sort_by_key(m_penalties.begin(), m_penalties.end(), m_bestIndexes.begin(), m_queue);
	}

	void BPSO::updateParticles()
	{
		const std::string source = krnl::updateParticles();

		compute::program program = compute::program::create_with_source(source, m_context);

		try
		{
			compute::uniform_real_distribution sigmoidDist(0.0f, 1.0f);
			sigmoidDist.generate(m_randomValues.begin(), m_randomValues.end(), *m_generator, m_queue);

			program.build();

			compute::kernel kernel(program, "update_particles");

			kernel.set_arg(0, m_legTypes);
			kernel.set_arg(1, m_sigmas);
			kernel.set_arg(2, m_randomValues);
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

	void BPSO::createCopyMask()
	{
		const std::string source = krnl::createCopyMask();

		compute::program program = compute::program::create_with_source(source, m_context);

		try
		{
			program.build();

			compute::kernel kernel(program, "create_copy_mask");

			kernel.set_arg(0, m_copyMask);
			kernel.set_arg(1, m_bestPenalties);
			kernel.set_arg(2, m_penalties);
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

	void BPSO::copyBestSolutions()
	{
		auto begin = boost::compute::make_zip_iterator(
			boost::make_tuple(m_copyMask.begin(), m_legTypes.begin(), m_bestLegTypes.begin())
		);

		auto end = boost::compute::make_zip_iterator(
			boost::make_tuple(m_copyMask.end(), m_legTypes.end(), m_bestLegTypes.end())
		);

		compute::transform(
			begin, end, m_bestLegTypes.begin(),
			(compute::lambda::get<0>(compute::lambda::_1) * compute::lambda::get<1>(compute::lambda::_1) +
				(1 - compute::lambda::get<0>(compute::lambda::_1)) * compute::lambda::get<2>(compute::lambda::_1)),
			m_queue
		);
	}

	void BPSO::calculateVelocities()
	{
		const std::string source = krnl::calculateVelocities();

		compute::program program = compute::program::create_with_source(source, m_context);

		try
		{
			program.build();

			compute::kernel kernel(program, "calculate_velocities");

			kernel.set_arg(0, m_legTypes);
			kernel.set_arg(1, m_bestLegTypes);
			kernel.set_arg(2, m_bestIndexes);
			kernel.set_arg(3, m_velocities);
			kernel.set_arg(4, m_randoms1);
			kernel.set_arg(5, m_randoms2);
			kernel.set_arg(6, m_w);
			kernel.set_arg(7, m_c1);
			kernel.set_arg(8, m_c2);
			kernel.set_arg(9, m_maxV);
			kernel.set_arg(10, m_netCount);

			m_queue.enqueue_1d_range_kernel(kernel, 0, m_populationSize, 0);

			m_queue.finish();
		}
		catch (const compute::opencl_error& e)
		{
			std::cerr << "OpenCL error: " << e.what() << std::endl;
			std::cout << program.build_log() << std::endl;
		}
	}

	void BPSO::calculateTransferFunction()
	{
		boost::compute::function<float(float)> sigmoid =
			boost::compute::make_function_from_source<float(float)>(
				"sigmoid",
				"float sigmoid(float x) { return 2.0f * fabs(1.0f / (1.0f + exp(-x)) - 0.5f); }"
			);
		compute::transform(m_velocities.begin(), m_velocities.end(), m_sigmas.begin(), sigmoid, m_queue);
	}

	OptimizationSolution BPSO::optimize()
	{
		createRandomParticles();

		for (unsigned int i = 0; i < m_generations; ++i)
		{
			calculatePenalties();
			createCopyMask();
			copyBestSolutions();
			findBestSolutions();
			calculateVelocities();
			calculateTransferFunction();
			updateParticles();
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

		for (size_t i = 0; i < penalties.size(); ++i)
		{
			std::cout << "penalty i " << i << " " << penalties[i] << '\n';
		}

		return { result , penalties[0] };
	}

}