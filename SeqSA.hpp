#pragma once

#include "IOptimizationSolver.hxx"
#include "NetSolution.hpp"

namespace in
{
    class SeqSA : public IOptimizationSolver
    {
    public:
        ~SeqSA() override = default;
        SeqSA(const GlobalRoutingGrid& globalGrid,
                const GlobalRoutingCells& startingGrid,
                const OptimizationRoutingData& initialSolutions,
                float initialTemperature,
                float coolingRate,
                float eps,
                size_t maxIterations);
        OptimizationRoutingData optimize() override;
    protected:
        void addSolution(GlobalRoutingCells& grid, const DoglegSegment& solution);
        void substractSolution(GlobalRoutingCells &grid, const DoglegSegment &solution);
        void addSolutions(GlobalRoutingCells& grid, const OptimizationRoutingData& solutions);
        float calculateCellPenalty(const GlobalRoutingCell& cell) const;
        float ripUpAndReroute(GlobalRoutingCells& grid, const NetSolution& oldSolution, const NetSolution& newSolution);
        float getCurrentPenalty() const;
        const GlobalRoutingGrid& m_globalGrid;
        GlobalRoutingCells m_startingGrid;
        OptimizationRoutingData m_initialSolutions;
        float m_initialTemperature{ 0.0f };
        float m_coolingRate{ 0.0f };
        float m_eps{ 0.0f };
        float m_penalty{ 0.0f };
        size_t m_maxIterations{ 0 };
    };
}