#include "GlobalRouter.hpp"
#include "DoglegRouter.hpp"

#include <execution>
#include <algorithm>
#include <random>

#include <omp.h>

#include "RNDPerTStepSpacePartitionedGPUSA.hpp"
#include "GeneticAlgorithm.hpp"
#include "GeneticAlgorithmRandRatio.hpp"
#include "EDA.hpp"

namespace in
{
	GlobalRouter::GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows)
		: m_dataTransformer{ dataTransformer }
	{
		m_placedPins = m_dataTransformer.getPlacedPins();
		m_globalGrid = m_dataTransformer.getGlobalGrid(cols, rows);
		m_grid = m_globalGrid.getGrid();
		m_netlist = m_globalGrid.getNetlist(m_placedPins);
		m_treeNetlist = m_dataTransformer.getMST();
		m_steinerTreeNetlist = m_dataTransformer.getRMST();
	}

	const TreeNetlist& GlobalRouter::getTreeNetlist() const
	{
		return m_treeNetlist;
	}

	const SteinerTreeNetlist& GlobalRouter::getSteinerTreeNetlist()
	{
		return m_steinerTreeNetlist;
	}

	std::map<int, int> GlobalRouter::getNetlistElementHistogram() const
	{
		std::map<int, int> hist;

		for (const auto& net : m_netlist)
		{
			hist[net.second.size()]++;
		}

		return hist;
	}

	const GlobalSolutions& GlobalRouter::getDoglegSolutions() const
	{
		return m_doglegSolutions;
	}

	const Netlist& GlobalRouter::getNetlist() const
	{
		return m_netlist;
	}

	void GlobalRouter::initializeDoglegTypes()
	{
		m_doglegTypes.resize(m_twoPointNets.size(), DoglegType::UPPER);
	}

	void GlobalRouter::readAllTwoPointNets()
	{
		m_twoPointNets.reserve(m_netlist.size());

		for (const auto& net : m_netlist)
		{
			if (net.second.size() != 2)
			{
				continue;
			}

			m_twoPointNets.push_back(net);
		}

		std::ranges::sort(m_twoPointNets, [](const auto& netA, const auto& netB)
			{
				const auto distanceA = manhattanDistance({ netA.second[0], netA.second[1] });
				const auto distanceB = manhattanDistance({ netB.second[0], netB.second[1] });
				return distanceA > distanceB;
			});

		for (const auto& net : m_twoPointNets)
		{
			std::cout << "Net " << net.first << " has manhattan distance " << manhattanDistance({ net.second[0], net.second[1] }) << std::endl;
		}
		m_twoPointNets.shrink_to_fit();
	}

	void GlobalRouter::readAllDoglegNets()
	{
		m_doglegSolutions.reserve(m_twoPointNets.size());

		for (size_t i = 0; i < m_twoPointNets.size(); ++i)
		{
			const auto& net = m_twoPointNets[i];
			if (manhattanDistance({ net.second[0], net.second[1] }) == 0)
			{
				continue;
			}
			NetSolution doglegNet;
			doglegNet.endpoints = { net.second[0], net.second[1] };
			doglegNet.name = net.first;
			doglegNet.type = m_doglegTypes[i];

			m_doglegSolutions.emplace_back(std::move(doglegNet));
		}
	}

	void GlobalRouter::placeNonSubjectToOptimizationNetsOnGrid()
	{
		for (size_t i = 0; i < m_nonOptimizationData.netNames.size(); ++i)
		{
			NetSolution solution;
			solution.endpoints = { {m_nonOptimizationData.globalNetStartsX[i],
				m_nonOptimizationData.globalNetStartsY[i]},
				{m_nonOptimizationData.globalNetEndsX[i],
				m_nonOptimizationData.globalNetEndsY[i]} };
			solution.name = m_nonOptimizationData.netNames[i];
			solution.type = m_nonOptimizationData.legTypes[i];

			const auto dogleg = route(solution);
			for (int i = dogleg.horizontalSegment.first.x(); i <= dogleg.horizontalSegment.second.x(); ++i)
			{
				auto& cell = m_grid.horizontalCells[m_globalGrid.getIndexHorizontal({ i,dogleg.horizontalSegment.first.y() })];
				cell.congestion++;
			}
			for (int i = dogleg.verticalSegment.first.y(); i <= dogleg.verticalSegment.second.y(); ++i)
			{
				auto& cell = m_grid.verticalCells[m_globalGrid.getIndexVertical({ dogleg.verticalSegment.first.x(),i })];
				cell.congestion++;
			}
		}
	}

	void GlobalRouter::readRoutingData()
	{
		for (const auto& steinerNet : m_steinerTreeNetlist.nets)
		{
			for (const auto& steinerSegment : steinerNet.segments)
			{
				m_routingData.netNames.push_back(steinerNet.name);
				m_routingData.legTypes.push_back(steinerSegment.type);
				m_routingData.netStartsX.push_back(steinerSegment.a.x());
				m_routingData.netStartsY.push_back(steinerSegment.a.y());
				m_routingData.netEndsX.push_back(steinerSegment.b.x());
				m_routingData.netEndsY.push_back(steinerSegment.b.y());

				const auto globalStartPos = m_globalGrid.getCoordinates(steinerSegment.a);
				const auto globalStartX = globalStartPos.x();
				const auto globalStartY = globalStartPos.y();
				const auto globalEndPos = m_globalGrid.getCoordinates(steinerSegment.b);
				const auto globalEndX = globalEndPos.x();
				const auto globalEndY = globalEndPos.y();
				m_routingData.globalNetStartsX.push_back(globalStartX);
				m_routingData.globalNetStartsY.push_back(globalStartY);
				m_routingData.globalNetEndsX.push_back(globalEndX);
				m_routingData.globalNetEndsY.push_back(globalEndY);
			}
		}
	}

	void GlobalRouter::fillDoglegTypes()
	{
		for (size_t i = 0; i < m_routingData.legTypes.size(); ++i)
		{
			m_routingData.legTypes[i] = DoglegType::LOWER;
		}
	}

	void GlobalRouter::readOptimizationData()
	{
		const auto maxSize = m_routingData.netNames.size();
		m_optimizationData.netNames.resize(maxSize);
		m_optimizationData.globalNetStartsX.resize(maxSize);
		m_optimizationData.globalNetStartsY.resize(maxSize);
		m_optimizationData.globalNetEndsX.resize(maxSize);
		m_optimizationData.globalNetEndsY.resize(maxSize);
		m_optimizationData.legTypes.resize(maxSize);

		m_nonOptimizationData.netNames.resize(maxSize);
		m_nonOptimizationData.globalNetStartsX.resize(maxSize);
		m_nonOptimizationData.globalNetStartsY.resize(maxSize);
		m_nonOptimizationData.globalNetEndsX.resize(maxSize);
		m_nonOptimizationData.globalNetEndsY.resize(maxSize);
		m_nonOptimizationData.legTypes.resize(maxSize);

		size_t i = 0;
		size_t k = 0;

		for (size_t j = 0; j < maxSize; ++j)
		{
			const auto rectDist = manhattanDistance({ {m_routingData.globalNetStartsX[j], m_routingData.globalNetStartsY[j]},
				{m_routingData.globalNetEndsX[j], m_routingData.globalNetEndsY[j]} });
			const auto chebDist = chebyshevDistance({ {m_routingData.globalNetStartsX[j], m_routingData.globalNetStartsY[j]},
				{m_routingData.globalNetEndsX[j], m_routingData.globalNetEndsY[j]} });

			if (rectDist == chebDist)
			{
				m_nonOptimizationData.netNames[k] = m_routingData.netNames[j];
				m_nonOptimizationData.globalNetStartsX[k] = m_routingData.globalNetStartsX[j];
				m_nonOptimizationData.globalNetStartsY[k] = m_routingData.globalNetStartsY[j];
				m_nonOptimizationData.globalNetEndsX[k] = m_routingData.globalNetEndsX[j];
				m_nonOptimizationData.globalNetEndsY[k] = m_routingData.globalNetEndsY[j];
				m_nonOptimizationData.legTypes[k] = m_routingData.legTypes[j];
				++k;
			}
			else
			{
				m_optimizationData.netNames[i] = m_routingData.netNames[j];
				m_optimizationData.globalNetStartsX[i] = m_routingData.globalNetStartsX[j];
				m_optimizationData.globalNetStartsY[i] = m_routingData.globalNetStartsY[j];
				m_optimizationData.globalNetEndsX[i] = m_routingData.globalNetEndsX[j];
				m_optimizationData.globalNetEndsY[i] = m_routingData.globalNetEndsY[j];
				m_optimizationData.legTypes[i] = m_routingData.legTypes[j];
				++i;
			}
		}

		m_optimizationData.netNames.resize(i);
		m_optimizationData.globalNetStartsX.resize(i);
		m_optimizationData.globalNetStartsY.resize(i);
		m_optimizationData.globalNetEndsX.resize(i);
		m_optimizationData.globalNetEndsY.resize(i);
		m_optimizationData.legTypes.resize(i);

		m_nonOptimizationData.netNames.resize(k);
		m_nonOptimizationData.globalNetStartsX.resize(k);
		m_nonOptimizationData.globalNetStartsY.resize(k);
		m_nonOptimizationData.globalNetEndsX.resize(k);
		m_nonOptimizationData.globalNetEndsY.resize(k);
		m_nonOptimizationData.legTypes.resize(k);
	}

	void GlobalRouter::createInitialSolution()
	{
		readRoutingData();
		fillDoglegTypes();
		readOptimizationData();
		placeNonSubjectToOptimizationNetsOnGrid();
	}

	const GlobalRoutingCells& GlobalRouter::getGrid() const
	{
		return m_grid;
	}

	void GlobalRouter::performSA(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate)
	{
		m_solver = std::make_unique<SeqSA>(
			m_globalGrid,
			m_grid,
			m_optimizationData,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations);

		std::cout << "Initialized simulated annealing on: " << m_optimizationData.netNames.size() << std::endl;

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performSAPar(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate)
	{
		m_solver = std::make_unique<ParSA>(
			m_globalGrid,
			m_grid,
			m_optimizationData,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations);

		std::cout << "Initialized parallel simulated annealing on: " << m_optimizationData.netNames.size() << std::endl;

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performSAParSpacePartitioned(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces)
	{
		m_solver = std::make_unique<SpacePartitionedSA>(
			m_globalGrid,
			m_grid,
			m_optimizationData,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations,
			spaces);

		std::cout << "Initialized space partitioned parallel simulated annealing on: " << m_optimizationData.netNames.size()
			<< " solutions in " << spaces << " spaces." << std::endl;

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performSAParSpacePartitionedOnGPU(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces)
	{
		m_solver = std::make_unique<SpacePartitionedGPUSA>(
			m_globalGrid,
			m_grid,
			m_optimizationData,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations,
			spaces);

		std::cout << "Initialized space partitioned parallel GPU simulated annealing on: " << m_optimizationData.netNames.size()
			<< " solutions in " << spaces << " spaces." << std::endl;

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performSAParSpacePartitionedOnGPUWithRandsPerIteration(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces)
	{
		m_solver = std::make_unique<RNDPerTStepSpacePartitionedGPUSA>(
			m_globalGrid,
			m_grid,
			m_optimizationData,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations,
			spaces);

		std::cout << "Initialized space partitioned parallel GPU simulated annealing on with rands generated per temperature iteration: " << m_optimizationData.netNames.size()
			<< " solutions in " << spaces << " spaces." << std::endl;

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performGeneticAlgorithm(unsigned int generations, unsigned int populationSize, float mutationRate)
	{
		m_solver = std::make_unique<GeneticAlgorithm>(m_globalGrid, m_grid, m_optimizationData, generations, populationSize, mutationRate);

		std::cout << "Initialized GeneticAlgorithmon\n";

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performGeneticAlgorithmRandRatio(unsigned int generations, unsigned int populationSize, float crossoverRate, float mutationRate)
	{
		m_solver = std::make_unique<GeneticAlgorithmRandRatio>(m_globalGrid, m_grid, m_optimizationData, generations, populationSize,crossoverRate, mutationRate);

		std::cout << "Initialized GeneticAlgorithmon\n";

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}

	void GlobalRouter::performEDA(unsigned int generations, unsigned int populationSize, float alpha, float limit)
	{
		m_solver = std::make_unique<EDA>(m_globalGrid, m_grid, m_optimizationData, generations, populationSize, alpha, limit);

		std::cout << "Initialized EDA\n";

		m_optimizationResult = m_solver->optimize();

		std::cout << "Optimized penalty: " << m_optimizationResult.penalty << '\n';
	}
}
