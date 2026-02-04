#pragma once

#include <random>

#include "DataTransformer.hpp"
#include "ISpecializedRouter.hxx"
#include "SeqSA.hpp"
#include "ParSA.hpp"
#include "SpacePartitionedSA.hpp"

namespace in
{

class IGlobalRouter
{
public:
	virtual ~IGlobalRouter() = default;
};

using Indices = GlobalRoutingGrid::Indices;
using Net = GlobalRoutingGrid::Net;
using Netlist = GlobalRoutingGrid::Netlist;
using Coord = GlobalRoutingGrid::Coordinates;

class GlobalRouter : public IGlobalRouter
{
public:
	
	GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows);
	~GlobalRouter() = default;
	void createInitialSolution();

	const Netlist& getNetlist() const;
	std::map<int, int> getNetlistElementHistogram() const;
	const GlobalSolutions& getDoglegSolutions() const;
	const GlobalRoutingCells& getGrid() const;

	void performSA(size_t maxIterations, float initialTemperature, float coolingRate);
	void performSAPar(size_t maxIterations, float initialTemperature, float coolingRate);
	void performSAParSpacePartitioned(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces);
private:
	DataTransformer& m_dataTransformer;
	std::vector<Pin> m_placedPins;
	GlobalRoutingGrid m_globalGrid;
	GlobalRoutingCells m_grid;
	std::vector<DoglegType> m_doglegTypes;
	Netlist m_netlist;
	Netlist m_twoPointNets;
	GlobalSolutions m_doglegSolutions;
	GlobalSolutions m_solutions;
	float d_penalty{ 0.0f };
	std::unique_ptr<IOptimizationSolver> m_solver;

	void readAllTwoPointNets();
	void initializeDoglegTypes();
	void routeAllDoglegNets();
	void placeDoglegPathsOnGrid();
};

}