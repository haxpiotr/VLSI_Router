#include "ParSA.hpp"

#include <omp.h>

#include <limits>

namespace in
{
    ParSA::ParSA(
		const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const GlobalSolutions& initialSolutions,
		float initialTemperature,
		float coolingRate,
		float eps,
		size_t maxIterations)
		: SeqSA(
			globalGrid,
			startingGrid,
			initialSolutions,
			initialTemperature,
			coolingRate,
			eps,
			maxIterations)
	{
	}

	GlobalSolutions ParSA::optimize()
	{
		float temperature = m_initialTemperature;
		auto globalBestSolutions = m_initialSolutions;
		float globalBestPenalty = std::numeric_limits<float>::max();

		while(temperature > m_eps)
		{
			#pragma omp parallel
			{
				thread_local std::random_device rd;
				thread_local std::mt19937 gen(rd() ^ omp_get_thread_num());
				thread_local auto localGrid = m_startingGrid; 
				thread_local auto localSolutions = createRandomSolutions(m_initialSolutions, gen);
				addSolutions(localGrid, localSolutions);
				float localCurrentPenalty = localGrid.penalty;
				const auto threadCount = omp_get_num_threads();
				
				for(size_t i = 0; i < m_maxIterations / threadCount; ++i)
				{
					const size_t candidateIndex = getUniform(gen, 0, localSolutions.size() - 1);
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

				#pragma omp critical
				{
					if (localCurrentPenalty < globalBestPenalty)
					{
						globalBestPenalty = localCurrentPenalty;
						globalBestSolutions = localSolutions;
					}
				}
			}

			temperature *= m_coolingRate;	
		}

		return globalBestSolutions;
	}
}