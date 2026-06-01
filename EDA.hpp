#pragma once

#include "IOptimizationSolver.hxx"

#include <boost/compute/container/vector.hpp>
#include <boost/compute/random.hpp>

namespace in
{

namespace bc= boost::compute;

class EDA : public IOptimizationSolver
{
public:
	EDA(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
		float alpha,
		float limit);
	OptimizationSolution optimize() override;

protected:
	void initDeviceData();
	void createRandomSolutions();
	void calculatePenalties();
	void selectSolution();
	void updateProbabilities();
	void createUpdatedSolutions();

	bc::device m_device;
	bc::context m_context;
	bc::command_queue m_queue;
	std::unique_ptr<bc::threefry_engine<>> m_generator;
	GlobalRoutingGrid m_globalGrid;
	GlobalRoutingCells m_startingGrid;
	OptimizationRoutingData m_solutionData;
	unsigned int m_generations;
	unsigned int m_populationSize;
	float m_alpha;
	float m_limit;
	unsigned int m_netCount;
	bc::buffer_iterator<float> m_iter;

	bc::vector<bc::int2_> m_netStarts;
	bc::vector<bc::int2_> m_netEnds;
	bc::vector<char> m_oldLegTypes;
	bc::vector<char> m_legTypes;
	bc::vector<char> m_bestSolution;
	bc::vector<float> m_penalties;
	bc::vector<float> m_randomValues;
	bc::vector<float> m_probabilities;
	bc::vector<int> m_startingHorizontalGrid;
	bc::vector<int> m_startingVerticalGrid;
	bc::vector<int> m_horizontalGrid;
	bc::vector<int> m_verticalGrid;
	bc::vector<unsigned int> m_bestIndexes;
};

}