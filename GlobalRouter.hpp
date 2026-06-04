#pragma once

#include <random>

#include "DataTransformer.hpp"
#include "ISpecializedRouter.hxx"
#include "SeqSA.hpp"
#include "ParSA.hpp"
#include "SpacePartitionedSA.hpp"
#include "SpacePartitionedGPUSA.hpp"
#include "RoutingDataTypes.hpp"

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
using NetLeg = std::pair<std::string, std::string>;

class GlobalRouter : public IGlobalRouter
{
public:
	
	GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows);
	~GlobalRouter() = default;

	void readRoutingData();
	void fillDoglegTypes();
	void readOptimizationData();
	void createInitialSolution();
	const Netlist& getNetlist() const;
	const TreeNetlist& getTreeNetlist() const;
	const SteinerTreeNetlist& getSteinerTreeNetlist();

	std::map<int, int> getNetlistElementHistogram() const;

	const GlobalSolutions& getDoglegSolutions() const;
	const GlobalRoutingCells& getGrid() const;

	void performSA(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate,
		float eps);
	void performSAPar(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate,
		float eps,
		int threads);
	void performSAParSpacePartitioned(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate,
		float eps,
		size_t spaces);
	void performSAParSpacePartitionedOnGPU(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate,
		float eps,
		size_t spaces);
	void performSAParSpacePartitionedOnGPUWithRandsPerIteration(
		size_t maxIterations,
		float initialTemperature,
		float coolingRate,
		float eps,
		size_t spaces);
	void performGeneticAlgorithm(
		unsigned int generations,
		unsigned int populationSize,
		float mutationRate);
    void performGeneticAlgorithmRandRatio(
		unsigned int generations,
		unsigned int populationSize,
		float crossoverRate,
		float mutationRate);
	void performEDA(
		unsigned int generations,
		unsigned int populationSize,
		float alpha,
		float limit);

private:
	DataTransformer& m_dataTransformer;
	std::vector<Pin> m_placedPins;
	GlobalRoutingGrid m_globalGrid;
	GlobalRoutingCells m_grid;
	std::vector<DoglegType> m_doglegTypes;
	Netlist m_netlist;
	Netlist m_twoPointNets;
	GlobalSolutions m_doglegSolutions;
	std::unique_ptr<IOptimizationSolver> m_solver;
	TreeNetlist m_treeNetlist;
	SteinerTreeNetlist m_steinerTreeNetlist;
	RoutingData m_routingData;
	OptimizationRoutingData m_optimizationData;
	OptimizationRoutingData m_nonOptimizationData;
	OptimizationSolution m_optimizationResult;

	void readAllTwoPointNets();
	void initializeDoglegTypes();
	void readAllDoglegNets();
	void placeNonSubjectToOptimizationNetsOnGrid();
};

}