#include "SpacePartitionedSA.hpp"

#include <omp.h>

namespace in
{
    SpacePartitionedSA::SpacePartitionedSA(
									   const GlobalRoutingGrid& globalGrid,
									   const GlobalRoutingCells& startingGrid,
									   const OptimizationRoutingData& initialSolutions,
									   float initialTemperature,
									   float coolingRate,
									   float eps,
									   size_t maxIterations,
									   size_t independentSpacesSize) : 
									   SeqSA(
										globalGrid,
										startingGrid,
										initialSolutions,
										initialTemperature,
										coolingRate,
										eps,
										maxIterations), m_independentSpacesSize{independentSpacesSize}
    {
        initializeIndependentSpaces();
    }

    void SpacePartitionedSA::initializeIndependentSpaces()
    {
		const auto indexRange = static_cast<size_t>(std::sqrt(m_independentSpacesSize));

		for(size_t mask = 0; mask < m_independentSpacesSize; ++mask)
		{
			OptimizationRoutingData space = m_initialSolutions;
			for(size_t i = 0; i < indexRange; ++i)
			{
				const bool iSet = (mask >> i) & 0x01;
				if(iSet)
				{
					space.legTypes[i] = flipDoglegType(space.legTypes[i]);
				}
			}
			m_spaces.emplace_back(std::move(space));
		}
    }

	OptimizationSolution SpacePartitionedSA::optimize()
	{
		auto globalBestSolutions = m_initialSolutions;
		float globalBestPenalty = std::numeric_limits<float>::max();
		std::vector<float> penalties(m_independentSpacesSize);

		#pragma omp parallel shared(globalBestPenalty, globalBestSolutions)
		{
			float temperature = m_initialTemperature;
			thread_local const auto threadNum = omp_get_thread_num();
			thread_local std::random_device rd;
			thread_local std::mt19937 gen(rd() ^ threadNum);
			thread_local auto localGrid = m_startingGrid; 
			thread_local auto localSolutions = m_spaces[threadNum];
			const auto threadCount = m_independentSpacesSize;
			const size_t startRange = static_cast<size_t>(std::sqrt(threadCount));
			addSolutions(localGrid,localSolutions);
			while(temperature > m_eps)
			{
				float localCurrentPenalty = localGrid.penalty;				
				
				for(size_t i = 0; i < m_maxIterations / threadCount; ++i)
				{
					const size_t candidateIndex = getUniform(gen, startRange, localSolutions.legTypes.size() - 1);				

					NetSolution previousSolution;
					previousSolution.name = localSolutions.netNames[candidateIndex];
					previousSolution.type = localSolutions.legTypes[candidateIndex];
					previousSolution.endpoints = { {localSolutions.globalNetStartsX[candidateIndex],localSolutions.globalNetStartsY[candidateIndex]},
						{localSolutions.globalNetEndsX[candidateIndex],localSolutions.globalNetEndsY[candidateIndex]} };

					auto candidateSolution = previousSolution;
					flipDoglegType(candidateSolution);

					const float newPenalty = ripUpAndReroute(localGrid, previousSolution, candidateSolution);
					const float deltaPenalty = newPenalty - localCurrentPenalty;

					if (deltaPenalty < 0.0f || std::exp(-deltaPenalty / temperature) > getUniform(gen, 0.0f, 1.0f))
					{
						localSolutions.legTypes[candidateIndex] = candidateSolution.type;
						localCurrentPenalty = newPenalty;
						penalties[threadNum] = localCurrentPenalty;
					}
					else
					{
						ripUpAndReroute(localGrid, candidateSolution, previousSolution);
					}
				}

				temperature *= m_coolingRate;	
			}

		}

		const auto minPenaltyIt = std::min_element(penalties.begin(), penalties.end());
		const auto resultLegTypesIndex = std::distance(penalties.begin(), minPenaltyIt);

		return { m_spaces[resultLegTypesIndex].legTypes, *minPenaltyIt };
	}
}