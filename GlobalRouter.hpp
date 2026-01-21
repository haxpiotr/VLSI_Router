#pragma once

#include "DataTransformer.hpp"

namespace in
{

class IGlobalRouter
{
public:
	virtual ~IGlobalRouter() = default;
};

class GlobalRouter : public IGlobalRouter
{
public:
	using Indices = GlobalRoutingGrid::Indices;
	using Net = GlobalRoutingGrid::Net;
	using Netlist = GlobalRoutingGrid::Netlist;
	using Coord = GlobalRoutingGrid::Coordinates;

	GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows);
	~GlobalRouter() = default;

	[[nodiscard]] const Netlist& getNetlist() const;
	[[nodiscard]] std::map<int, int> getNetlistElementHistogram() const;
	[[nodiscard]] std::vector<Coord> performUpperDogleg(const Coord& a, const Coord& b) const;
	[[nodiscard]] std::vector<Coord> performLowerDogleg(const Coord& a, const Coord& b) const;
	[[nodiscard]] bool verifyForDogleg(const Net& net) const;
	void routeAllDoglegNets();
	[[nodiscard]] const std::vector<std::vector<Coord>>& getDoglegPaths() const;
	void placeDoglegPathsOnGrid();
	[[nodiscard]] const std::vector<GlobalRoutingCell>& getGrid() const;

private:
	DataTransformer& m_dataTransformer;
	std::vector<Pin> m_placedPins;
	GlobalRoutingGrid m_globalGrid;
	std::vector<GlobalRoutingCell> m_grid;
	Netlist m_netlist;
	std::vector<std::vector<Coord>> m_doglegPaths;
	
};

}