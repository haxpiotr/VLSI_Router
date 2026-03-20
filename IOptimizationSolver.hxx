#pragma once

#include "NetSolution.hpp"
#include "RoutingDataTypes.hpp"

namespace in
{

class IOptimizationSolver
{
public:
	virtual ~IOptimizationSolver() = default;
	virtual OptimizationSolution optimize() = 0;
};

}
