#pragma once

#include <string>
#include <vector>
#include "GeometryTypes.hpp"

namespace in
{
	struct RoutingData
	{
		std::vector<std::string> netNames;
		std::vector<int> netStartsX;
		std::vector<int> netStartsY;
		std::vector<int> netEndsX;
		std::vector<int> netEndsY;
		std::vector<int> globalNetStartsX;
		std::vector<int> globalNetStartsY;
		std::vector<int> globalNetEndsX;
		std::vector<int> globalNetEndsY;
		std::vector<DoglegType> legTypes;
	};

	struct OptimizationRoutingData
	{
		std::vector<std::string> netNames;
		std::vector<int> globalNetStartsX;
		std::vector<int> globalNetStartsY;
		std::vector<int> globalNetEndsX;
		std::vector<int> globalNetEndsY;
		std::vector<DoglegType> legTypes;
	};

	struct OptimizationSolution
	{
		std::vector<DoglegType> legTypes;
		GlobalRoutingCells grid;
		double penalty{ 0 };
	};

}