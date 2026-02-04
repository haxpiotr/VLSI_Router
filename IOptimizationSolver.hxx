#pragma once

#include "NetSolution.hpp"

namespace in
{

class IOptimizationSolver
{
public:
	virtual ~IOptimizationSolver() = default;
	virtual GlobalSolutions optimize() = 0;
};

}
