#include "SeqSA.hpp"

namespace in
{
    SeqSA::SeqSA(
    const GlobalRoutingGrid& globalGrid,
    const GlobalRoutingCells& startingGrid,
    const OptimizationRoutingData& initialSolutions,
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

	void SeqSA::addSolution(GlobalRoutingCells &grid, const DoglegSegment& solution)
	{
        const auto startVerticalIndex = solution.verticalSegment.first.y();
        const auto endVerticalIndex = solution.verticalSegment.second.y();

        for (auto i = startVerticalIndex; i <= endVerticalIndex; ++i)
        {
            const auto index = m_globalGrid.getIndexVertical({ solution.verticalSegment.first.x(), i });
            const auto oldCellPenalty = calculateCellPenalty(grid.verticalCells[index]);
            grid.verticalCells[index].congestion++;
            const auto newCellPenalty = calculateCellPenalty(grid.verticalCells[index]);
            grid.penalty += newCellPenalty - oldCellPenalty;
        }

        const auto startHorizontalIndex = solution.horizontalSegment.first.x();
        const auto endHorizontalIndex = solution.horizontalSegment.second.x();

        for (auto i = startHorizontalIndex; i <= endHorizontalIndex; ++i)
        {
            const auto index = m_globalGrid.getIndexHorizontal({ i, solution.horizontalSegment.first.y() });
            const auto oldCellPenalty = calculateCellPenalty(grid.horizontalCells[index]);
            grid.horizontalCells[index].congestion++;
            const auto newCellPenalty = calculateCellPenalty(grid.horizontalCells[index]);
            grid.penalty += newCellPenalty - oldCellPenalty;
        }
        
	}

	void SeqSA::substractSolution(GlobalRoutingCells &grid, const DoglegSegment& solution)
	{
      const auto startVerticalIndex = solution.verticalSegment.first.y();
      const auto endVerticalIndex = solution.verticalSegment.second.y();

      for (auto i = startVerticalIndex; i <= endVerticalIndex; ++i)
      {
        const auto index = m_globalGrid.getIndexVertical({ solution.verticalSegment.first.x(), i });
        const auto oldCellPenalty = calculateCellPenalty(grid.verticalCells[index]);
        grid.verticalCells[index].congestion--;
        const auto newCellPenalty = calculateCellPenalty(grid.verticalCells[index]);
        grid.penalty += newCellPenalty - oldCellPenalty;
      }

      const auto startHorizontalIndex = solution.horizontalSegment.first.x();
      const auto endHorizontalIndex = solution.horizontalSegment.second.x();

      for (auto i = startHorizontalIndex; i <= endHorizontalIndex; ++i)
      {
        const auto index =
          m_globalGrid.getIndexHorizontal({ i, solution.horizontalSegment.first.y() });
        const auto oldCellPenalty = calculateCellPenalty(grid.horizontalCells[index]);
        grid.horizontalCells[index].congestion--;
        const auto newCellPenalty = calculateCellPenalty(grid.horizontalCells[index]);
        grid.penalty += newCellPenalty - oldCellPenalty;
      }
	}

	void SeqSA::addSolutions(GlobalRoutingCells& grid, const OptimizationRoutingData& solutions)
	{
        for (size_t i = 0; i < solutions.legTypes.size(); ++i)
        {
            NetSolution solution;
            solution.type = solutions.legTypes[i];
            solution.name = solutions.netNames[i];
            solution.endpoints = { {solutions.globalNetStartsX[i],solutions.globalNetStartsY[i]},{solutions.globalNetEndsX[i],solutions.globalNetEndsY[i]}};
            addSolution(grid, route(solution));
        }
	}

	float SeqSA::calculateCellPenalty(const GlobalRoutingCell& cell) const
	{
		return static_cast<float>(std::pow(cell.congestion, 2));
	}

	float SeqSA::ripUpAndReroute(GlobalRoutingCells& grid, const NetSolution& oldSolution, const NetSolution& newSolution)
	{
		substractSolution(grid, route(oldSolution));
		addSolution(grid, route(newSolution));

		return grid.penalty;
	}

    OptimizationSolution SeqSA::optimize()
    {
        static std::mt19937 gen(2027);
        float temperature = m_initialTemperature;
        auto currentSolutions = m_initialSolutions;
        auto bestSolution = currentSolutions;
        
        addSolutions(m_startingGrid, m_initialSolutions);

        float currentPenalty = m_startingGrid.penalty;

        std::cout << "Initial Penalty: " << currentPenalty << std::endl;

        while(temperature > m_eps)
        {
            for(size_t i = 0; i < m_maxIterations; ++i)
            {
                const size_t candidateIndex = getUniform(gen, 0, currentSolutions.legTypes.size() - 1);

                NetSolution previousSolution;
                previousSolution.name = currentSolutions.netNames[candidateIndex];
                previousSolution.type = currentSolutions.legTypes[candidateIndex];
                previousSolution.endpoints = { {currentSolutions.globalNetStartsX[candidateIndex],currentSolutions.globalNetStartsY[candidateIndex]},
                    {currentSolutions.globalNetEndsX[candidateIndex],currentSolutions.globalNetEndsY[candidateIndex]} };

                auto candidateSolution = previousSolution;

                flipDoglegType(candidateSolution);

                const float newPenalty = ripUpAndReroute(m_startingGrid, previousSolution, candidateSolution);
                const float deltaPenalty = newPenalty - currentPenalty;

                if (deltaPenalty < 0.0f || std::exp(-deltaPenalty / temperature) > getUniform(gen, 0.0f, 1.0f))
                {
                    currentSolutions.legTypes[candidateIndex] = candidateSolution.type;
                    currentPenalty = newPenalty;
                }
                else
                {
                    ripUpAndReroute(m_startingGrid, candidateSolution, previousSolution);
                }
            }

            temperature *= m_coolingRate;
        }

        return { currentSolutions.legTypes, m_startingGrid, currentPenalty };
    }

	float SeqSA::getCurrentPenalty() const
	{
		return m_penalty;
	}
}