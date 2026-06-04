#include "ParSA.hpp"

#include <omp.h>

#include <limits>

namespace in
{
    ParSA::ParSA(
		const GlobalRoutingGrid& globalGrid,
		const GlobalRoutingCells& startingGrid,
		const OptimizationRoutingData& initialSolutions,
		float initialTemperature,
		float coolingRate,
		float eps,
		int threads,
		size_t maxIterations)
		: SeqSA(
			globalGrid,
			startingGrid,
			initialSolutions,
			initialTemperature,
			coolingRate,
			eps,
			maxIterations), 
		m_threads{ threads }
	{
	}

	OptimizationSolution ParSA::optimize()
	{
		float temperature = m_initialTemperature;
		auto globalBestSolutions = m_initialSolutions;
		float globalBestPenalty = std::numeric_limits<float>::max();
		auto globalBestGrid = m_startingGrid;
		addSolutions(globalBestGrid, globalBestSolutions);

		while(temperature > m_eps)
		{
			#pragma omp parallel
			{
				thread_local std::random_device rd;
				thread_local std::mt19937 gen(rd() ^ omp_get_thread_num());
				thread_local auto localGrid = globalBestGrid;
				thread_local auto localSolutions = globalBestSolutions;
				
				float localCurrentPenalty = localGrid.penalty;
				auto threadCount = std::min(omp_get_num_threads(), m_threads);
                threadCount = std::max(threadCount, 1);
				
				for(size_t i = 0; i < m_maxIterations / threadCount; ++i)
				{
					const size_t candidateIndex = getUniform(gen, 0, localSolutions.netNames.size() - 1);

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
					}
					else
					{
						ripUpAndReroute(localGrid, candidateSolution, previousSolution);
					}
				}

				#pragma omp critical
				{
					if (localGrid.penalty < globalBestPenalty)
					{
						globalBestPenalty = localCurrentPenalty;
						globalBestSolutions = localSolutions;
						globalBestGrid = localGrid;
					}
				}
			}
			temperature *= m_coolingRate;	
		}

		return { globalBestSolutions.legTypes, globalBestPenalty };
	}
}