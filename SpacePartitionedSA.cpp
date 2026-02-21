#include "SpacePartitionedSA.hpp"

#include <omp.h>

namespace in
{
    SpacePartitionedSA::SpacePartitionedSA(
									   const GlobalRoutingGrid& globalGrid,
									   const GlobalRoutingCells& startingGrid,
									   const GlobalSolutions& initialSolutions,
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
        auto sortedSolutions = m_initialSolutions;

        std::ranges::sort(sortedSolutions,[](const auto& netSolA, const auto& netSolB)
		{
			return manhattanDistance(netSolA.endpoints) > manhattanDistance(netSolA.endpoints);
		});

		//sortedSolutions = m_initialSolutions;
		
		const auto indexRange = static_cast<size_t>(std::sqrt(m_independentSpacesSize));

		for(size_t mask = 0; mask < m_independentSpacesSize; ++mask)
		{
			GlobalSolutions space = sortedSolutions;
			for(size_t i = 0; i < indexRange; ++i)
			{
				const bool iSet = (mask >> i) & 0x01;
				if(iSet)
				{
					flipDoglegType(space[i]);
				}
			}
			m_spaces.emplace_back(std::move(space));
		}
    }

	GlobalSolutions SpacePartitionedSA::optimize()
	{
		auto globalBestSolutions = m_initialSolutions;
		float globalBestPenalty = std::numeric_limits<float>::max();

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
					const size_t candidateIndex = getUniform(gen, startRange, localSolutions.size() - 1);
					const auto previousSolution = localSolutions[candidateIndex];
					auto candidateSolution = previousSolution;

					flipDoglegType(candidateSolution);

					const float newPenalty = ripUpAndReroute(localGrid, previousSolution, candidateSolution);
					const float deltaPenalty = newPenalty - localCurrentPenalty;

					if (deltaPenalty < 0.0f || std::exp(-deltaPenalty / temperature) > getUniform(gen, 0.0f, 1.0f))
					{
						localSolutions[candidateIndex] = candidateSolution;
						localCurrentPenalty = newPenalty;
					}
					else
					{
						ripUpAndReroute(localGrid, candidateSolution, previousSolution);
					}
				}

				temperature *= m_coolingRate;	
			}

			#pragma omp critical
			{
				if (localGrid.penalty < globalBestPenalty)
				{
					globalBestPenalty = localGrid.penalty;
					globalBestSolutions = localSolutions;
				}
			}

			std::cout << "Temperature: " << temperature << ", Current Penalty: " << globalBestPenalty << std::endl;
		}

		return globalBestSolutions;
	}
}