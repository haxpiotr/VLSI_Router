#include "SeqSA.hpp"

namespace in
{
    SeqSA::SeqSA(
    const GlobalRoutingGrid& globalGrid,
    const GlobalRoutingCells& startingGrid,
    const GlobalSolutions& initialSolutions,
    float initialTemperature,
    float coolingRate,
    float eps,
    size_t maxIterations)
    : m_globalGrid{ globalGrid }
    , m_startingGrid{ startingGrid }
    , m_initialSolutions{ initialSolutions }
    , m_initialTemperature{ initialTemperature }
    , m_coolingRate{ coolingRate }
    , m_eps{ eps }
    , m_maxIterations{ maxIterations }
	{
	}

	void SeqSA::addSolution(GlobalRoutingCells& grid, const NetSolution& solution)
	{
		for (const auto& coord : solution.path)
		{
			const auto index = m_globalGrid.getIndex(coord);
			const auto oldCellPenalty = calculateCellPenalty(grid.cells[index]);
			grid.cells[index].overallCongestion++;
			const auto newCellPenalty = calculateCellPenalty(grid.cells[index]);
			grid.penalty += newCellPenalty - oldCellPenalty;
		}
	}
	void SeqSA::substractSolution(GlobalRoutingCells& grid, const NetSolution& solution)
	{
		for (const auto& coord : solution.path)
		{
			const auto index = m_globalGrid.getIndex(coord);
			const auto oldCellPenalty = calculateCellPenalty(grid.cells[index]);
			grid.cells[index].overallCongestion--;
			const auto newCellPenalty = calculateCellPenalty(grid.cells[index]);
			grid.penalty += newCellPenalty - oldCellPenalty;
		}
	}

	void SeqSA::addSolutions(GlobalRoutingCells& grid, const GlobalSolutions& solutions)
	{
		for (const auto& solution : solutions)
		{
			addSolution(grid, solution);
		}
	}

	float SeqSA::calculateCellPenalty(const GlobalRoutingCell& cell) const
	{
		return static_cast<float>(std::pow(cell.overallCongestion, 2));
	}

	float SeqSA::ripUpAndReroute(GlobalRoutingCells& grid, const NetSolution& oldSolution, const NetSolution& newSolution)
	{
		substractSolution(grid, oldSolution);
		addSolution(grid, newSolution);

		return grid.penalty;
	}

	GlobalSolutions SeqSA::optimize()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        float temperature = m_initialTemperature;
        auto currentSolutions = m_initialSolutions;
        auto bestSolution = currentSolutions;
        
        addSolutions(m_startingGrid, m_initialSolutions);

        float currentPenalty = m_startingGrid.penalty;

        while(temperature > m_eps)
        {
            for(size_t i = 0; i < m_maxIterations; ++i)
            {
                const size_t candidateIndex = getUniform(gen, 0, currentSolutions.size() - 1);
                const auto previousSolution = currentSolutions[candidateIndex];
                auto candidateSolution = previousSolution;

                flipDoglegType(candidateSolution);
                route(candidateSolution);

                const float newPenalty = ripUpAndReroute(m_startingGrid, previousSolution, candidateSolution);
                const float deltaPenalty = newPenalty - currentPenalty;

                if (deltaPenalty < 0.0f || std::exp(-deltaPenalty / temperature) > getUniform(gen, 0.0f, 1.0f))
                {
                    currentSolutions[candidateIndex] = candidateSolution;
                    currentPenalty = newPenalty;
                }
                else
                {
                    ripUpAndReroute(m_startingGrid, candidateSolution, previousSolution);
                }
            }

            temperature *= m_coolingRate;
        }

        return currentSolutions;
    }

	float SeqSA::getCurrentPenalty() const
	{
		return m_penalty;
	}
}