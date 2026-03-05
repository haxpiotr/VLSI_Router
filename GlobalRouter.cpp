#include "GlobalRouter.hpp"
#include "DoglegRouter.hpp"

#include <execution>
#include <algorithm>
#include <random>

#include <omp.h>

namespace in
{
	GlobalRouter::GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows)
		: m_dataTransformer{ dataTransformer }
	{
		m_placedPins = m_dataTransformer.getPlacedPins();
		m_globalGrid = m_dataTransformer.getGlobalGrid(cols, rows);
		m_grid = m_globalGrid.getGrid();
		m_netlist = m_globalGrid.getNetlist(m_placedPins);
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

	void GlobalRouter::placeNetsOnGrid()
	{
		for (size_t i = 0; i < m_twoPointNets.size(); ++i)
		{
			const auto& twoPointNet = m_twoPointNets[i];
			NetSolution twoPointSolution;
			twoPointSolution.endpoints ={ twoPointNet.second[0], twoPointNet.second[1] };
			twoPointSolution.name = twoPointNet.first;
			twoPointSolution.type = m_doglegTypes[i];

			const auto dogleg = route(twoPointSolution);
			for (int i = dogleg.horizontalSegment.first.x(); i <= dogleg.horizontalSegment.second.x(); ++i)
			{
				auto& cell = m_grid.horizontalCells[m_globalGrid.getIndexHorizontal({i, twoPointSolution.endpoints.first.y()})];
				cell.congestion++;
			}
			for (int i = dogleg.verticalSegment.first.y(); i <= dogleg.verticalSegment.second.y(); ++i)
			{
				auto& cell = m_grid.verticalCells[m_globalGrid.getIndexVertical({ twoPointSolution.endpoints.first.x(), i})];
				cell.congestion++;
			}
		}
	}

	void GlobalRouter::createInitialSolution()
	{
		readAllTwoPointNets();
		initializeDoglegTypes();
		readAllDoglegNets();
		placeNetsOnGrid();
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
			m_globalGrid.getGrid(),
			m_doglegSolutions,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations);

		std::cout << "Initialized simulated annealing on: " << m_doglegSolutions.size() << std::endl;

		m_doglegSolutions = m_solver->optimize();
	}

	void GlobalRouter::performSAPar(
		size_t maxIterations, 
		float initialTemperature, 
		float coolingRate)
	{
		m_solver = std::make_unique<ParSA>(
			m_globalGrid,
			m_globalGrid.getGrid(),
			m_doglegSolutions,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations);

		std::cout << "Initialized parallel simulated annealing on: " << m_doglegSolutions.size() << std::endl;

		m_doglegSolutions = m_solver->optimize();
	}

	void GlobalRouter::performSAParSpacePartitioned(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces)
	{
		m_solver = std::make_unique<SpacePartitionedSA>(
			m_globalGrid,
			m_globalGrid.getGrid(),
			m_doglegSolutions,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations,
			spaces);

		std::cout << "Initialized space partitioned parallel simulated annealing on: " << m_doglegSolutions.size() 
			<< " solutions in " << spaces << " spaces." << std::endl;

		m_doglegSolutions = m_solver->optimize();
	}

	void GlobalRouter::performSAParSpacePartitionedOnGPU(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces)
	{
		m_solver = std::make_unique<SpacePartitionedGPUSA>(
			m_globalGrid,
			m_globalGrid.getGrid(),
			m_doglegSolutions,
			initialTemperature,
			coolingRate,
			0.001f,
			maxIterations,
			spaces);

		std::cout << "Initialized space partitioned parallel GPU simulated annealing on: " << m_doglegSolutions.size()
			<< " solutions in " << spaces << " spaces." << std::endl;

		m_doglegSolutions = m_solver->optimize();
	}
}
