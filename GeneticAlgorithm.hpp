#pragma once

#include "IOptimizationSolver.hxx"

#include <boost/compute/container/vector.hpp>

namespace in
{

namespace compute = boost::compute;

class GeneticAlgorithm : public IOptimizationSolver
{
public:
	~GeneticAlgorithm() = default;
	GeneticAlgorithm(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
		float mutationRate);
	OptimizationSolution optimize() override;

protected:
	void createRandomPopulation();
	void calculatePenalties();
	void findBestSolutions();
	void crossover();
	void initDeviceData();

private:
	GlobalRoutingGrid m_globalRoutingGrid;
	GlobalRoutingCells m_startingGrid;
	OptimizationRoutingData m_solutionData;
	unsigned int m_generations;
	unsigned int m_populationSize;
	float mutationRate;
	unsigned int m_netCount;
	
	compute::device m_device;
	compute::context m_context;
	compute::command_queue m_queue;
	compute::vector<compute::int2_> m_netStarts;
	compute::vector<compute::int2_> m_netEnds;
	compute::vector<char> m_oldLegTypes;
	compute::vector<char> m_legTypes;
	compute::vector<float> m_penalties;
	compute::vector<float> m_randomValues;
	compute::vector<int> m_startingHorizontalGrid;
	compute::vector<int> m_startingVerticalGrid;
	compute::vector<int> m_horizontalGrid;
	compute::vector<int> m_verticalGrid;
	compute::vector<unsigned int> m_bestIndexes;

};

}