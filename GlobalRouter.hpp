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

private:

	std::vector<Coord> performUpperDogleg(const Net& net) const;
	std::vector<Coord> performLowerDogleg(const Net& net) const;

	DataTransformer& m_dataTransformer;
	std::vector<Pin> m_placedPins;
	GlobalRoutingGrid m_globalGrid;
	std::vector<GlobalRoutingCell> m_grid;
	Netlist m_netlist;
	
};

}