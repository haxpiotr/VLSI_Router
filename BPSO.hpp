#pragma once

#include "IOptimizationSolver.hxx"

#include <boost/compute/container/vector.hpp>
#include <boost/compute/random.hpp>

namespace in
{

namespace compute = boost::compute;

class BPSO : public IOptimizationSolver
{
public:
	~BPSO() = default;
	BPSO(const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& solutionData,
		unsigned int generations,
		unsigned int populationSize,
		float w,
		float c_1,
		float c_2,
		float maxV);
	OptimizationSolution optimize() override;
protected:
	void initDeviceData();
	void createRandomParticles();
	void calculatePenalties();
	void createCopyMask();
	void copyBestSolutions();
	void calculateTransferFunction();
	void findBestSolutions();
	void updateBestSolutions();
	void calculateVelocities();
	void updateParticles();
	GlobalRoutingGrid m_globalRoutingGrid;
	GlobalRoutingCells m_startingGrid;
	OptimizationRoutingData m_solutionData;
	unsigned int m_generations;
	unsigned int m_populationSize;
	float m_w;
	float m_c1;
	float m_c2;
	float m_maxV;
	unsigned int m_netCount;

	compute::device m_device;
	compute::context m_context;
	compute::command_queue m_queue;
	std::unique_ptr<compute::mt19937> m_generator{ nullptr };
	compute::vector<compute::int2_> m_netStarts;
	compute::vector<compute::int2_> m_netEnds;
	compute::vector<char> m_bestLegTypes;
	compute::vector<char> m_legTypes;
	compute::vector<float> m_bestPenalties;
	compute::vector<float> m_penalties;
	compute::vector<float> m_randomValues;
	compute::vector<float> m_randoms1;
	compute::vector<float> m_randoms2;
	compute::vector<float> m_velocities;
	compute::vector<int> m_startingHorizontalGrid;
	compute::vector<int> m_startingVerticalGrid;
	compute::vector<int> m_horizontalGrid;
	compute::vector<int> m_verticalGrid;
	compute::vector<unsigned int> m_bestIndexes;
	compute::vector<char> m_copyMask;
	compute::vector<float> m_sigmas;
};

}