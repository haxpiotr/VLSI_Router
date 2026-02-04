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
		auto key_comparator = [](const auto& a, const auto& b)
			{
				return a.compIdPair < b.compIdPair;
			};

		std::sort(std::execution::par, m_placedPins.begin(), m_placedPins.end(), key_comparator);

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

		m_twoPointNets.shrink_to_fit();
	}

	void GlobalRouter::routeAllDoglegNets()
	{
		m_doglegSolutions.reserve(m_twoPointNets.size());
		m_doglegSolutions.resize(m_twoPointNets.size());

		for (size_t i = 0; i < m_twoPointNets.size(); ++i)
		{
			const auto& net = m_twoPointNets[i];
			m_doglegSolutions[i].path = createDoglegRouter(m_doglegTypes[i])->route(net);
			m_doglegSolutions[i].name = net.first;
			m_doglegSolutions[i].type = m_doglegTypes[i];
		}
	}

	void GlobalRouter::placeDoglegPathsOnGrid()
	{
		for (const auto& solution : m_doglegSolutions)
		{
			for(const auto& coord : solution.path)
			{
				const auto index = m_globalGrid.getIndex(coord);
				m_grid.cells[index].overallCongestion++;
			}
		}
	}

	void GlobalRouter::createInitialSolution()
	{
		readAllTwoPointNets();
		initializeDoglegTypes();
		routeAllDoglegNets();
		placeDoglegPathsOnGrid();
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
		m_doglegSolutions = m_solver->optimize();
	}
}
