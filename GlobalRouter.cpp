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

	std::vector<GlobalRouter::Coord> GlobalRouter::performUpperDogleg(const Net& net) const
	{
		const auto& [start, end] = std::minmax(net.second[0],net.second[1]);
		const auto verticalDistance = end.second - start.second;

		[[maybe_unused]]const auto step = verticalDistance > 0 ? 1 : -1;

		std::vector<Coord> path;

		return path;
	}
	std::vector<GlobalRouter::Coord> GlobalRouter::performLowerDogleg(const Net& net) const
	{
		const auto& [start, end] = std::minmax(net.second[0],net.second[1]);
		const auto verticalDistance = end.second - start.second;

		[[maybe_unused]]const auto step = verticalDistance > 0 ? 1 : -1;

		std::vector<Coord> path;

		return path;
	}

}


