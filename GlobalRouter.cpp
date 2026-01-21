#include "GlobalRouter.hpp"

#include <execution>

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

	const GlobalRouter::Netlist& GlobalRouter::getNetlist() const
	{
			return m_netlist;
	}

	bool GlobalRouter::verifyForDogleg(const Net& net) const
	{
		if (net.second.size() != 2)
		{
			return false;
		}

		return true;
	}

	std::vector<GlobalRouter::Coord> GlobalRouter::performUpperDogleg(const Coord& a, const Coord& b) const
	{		
		auto compare = [](const Coord& a, const Coord& b)
			{
				return a.y() < b.y();
			};
		const auto& [start, end] = std::minmax(a, b, compare);
		const auto verticalDistance = end.y() - start.y();
		const auto horizontalDistance = end.x() - start.x();

		const int verticalStep {1};
		const int horizontalStep = horizontalDistance > 0 ? 1 : -1;

		const size_t pathSize = 1 + std::abs(verticalDistance) + std::abs(horizontalDistance);

		std::vector<Coord> path(pathSize);

		auto it = std::generate_n(path.begin(), std::abs(verticalDistance), [&,currentY = start.y()]() mutable
		{
			Coord coord{ start.x(), currentY};
			currentY += verticalStep;
			return coord;
		});

		std::generate(it, path.end(), [&,currentX = start.x()]() mutable
		{
			Coord coord{ currentX, start.y() + verticalDistance };
			currentX += horizontalStep;
			return coord;
		});

		return path;
	}

	std::vector<GlobalRouter::Coord> GlobalRouter::performLowerDogleg(const Coord& a, const Coord& b) const
	{
		auto compare = [](const Coord& a, const Coord& b)
			{
				return a.y() < b.y();
			};
		const auto& [start, end] = std::minmax(a, b, compare);
		const auto verticalDistance = end.y() - start.y();
		const auto horizontalDistance = end.x() - start.x();

		const int verticalStep {1};
		const int horizontalStep = horizontalDistance > 0 ? 1 : -1;

		const size_t pathSize = 1 + std::abs(verticalDistance) + std::abs(horizontalDistance);

		std::vector<Coord> path(pathSize);

		auto it = std::generate_n(path.begin(), std::abs(horizontalDistance), [&,currentX = start.x()]() mutable
		{
			Coord coord{ currentX, start.y() };
			currentX += horizontalStep;
			return coord;
		});

		std::generate(it, path.end(), [&,currentX = start.x() + horizontalDistance, currentY = start.y()]() mutable
		{
			Coord coord{ currentX, currentY};
			currentY += verticalStep;
			return coord;
		});

		return path;
	}

	void GlobalRouter::routeAllDoglegNets()
	{
		std::vector<Net> doglegNets;
		doglegNets.reserve(m_netlist.size());

		for (const auto& net : m_netlist)
		{
			if (verifyForDogleg(net))
			{
				doglegNets.push_back(net);
			}
		}

		std::vector<std::vector<Coord>> doglegPaths;
		doglegPaths.reserve(doglegNets.size());

		for (const auto& net : doglegNets)
		{
			if(net.second[0].x()>=100 || net.second[0].y()>=100 ||
			   net.second[1].x()>=100 || net.second[1].y()>=100)
			{
				std::cout << "Coordinates out of bounds! on net: " << net.first << "\n";
			}
			
			const auto upperDoglegPath = performUpperDogleg(net.second[0], net.second[1]);
			doglegPaths.push_back(upperDoglegPath);
		}

		m_doglegPaths = doglegPaths;
	}

	void GlobalRouter::placeDoglegPathsOnGrid()
	{
		for (const auto& path : m_doglegPaths)
		{
			for(const auto& coord : path)
			{
				const auto index = m_globalGrid.getIndex(coord);
				m_grid[index].horizontalCongestion++;
			}
		}
	}

	const std::vector<GlobalRoutingCell>& GlobalRouter::getGrid() const
	{
		return m_grid;
	}

	const std::vector<std::vector<GlobalRouter::Coord>>& GlobalRouter::getDoglegPaths() const 
	{
		return m_doglegPaths;
	}

}


