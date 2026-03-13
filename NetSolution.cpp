#include "NetSolution.hpp"
#include "DoglegRouter.hpp"

namespace in
{
    float getUniform(std::mt19937& gen, float from, float to)
	{
		std::uniform_real_distribution<float> dis(from, to);
		return dis(gen);
	}

	size_t getUniform(std::mt19937& gen, size_t from, size_t to)
	{
		std::uniform_int_distribution<size_t> dis(from, to);
		return dis(gen);
	}

	void flipDoglegType(NetSolution& solution)
	{
		if (solution.type == DoglegType::UPPER)
		{
			solution.type = DoglegType::LOWER;
		}
		else
		{
			solution.type = DoglegType::UPPER;
		}
	}

	DoglegType flipDoglegType(DoglegType type)
	{
		if (type == DoglegType::UPPER)
		{
			return DoglegType::LOWER;
		}
		
		return DoglegType::UPPER;
	}

	GlobalSolutions createRandomSolutions(
		const GlobalSolutions& solutions,
		std::mt19937& gen)
	{
		auto randoms = solutions;
        
        std::uniform_int_distribution<int> dist(0, 1);

        for (auto& solution : randoms)
        {
            solution.type = (dist(gen) == 0) ? DoglegType::UPPER : DoglegType::LOWER;
        }

        return randoms;
	}
	

	DoglegSegment route(const NetSolution& solution)
	{
          return createDoglegRouter(solution.type)
            ->route({ solution.name, { solution.endpoints.first, solution.endpoints.second} });
	}

	int manhattanDistance(const Segment& segment)
	{ 
		const auto &[start, end] = segment;
		return std::abs(start.x() - end.x()) + std::abs(start.y() - end.y());
	}

	int chebyshevDistance(const Segment& segment)
	{
		const auto& [start, end] = segment;
		return std::max(std::abs(start.x() - end.x()), std::abs(start.y() - end.y()));
	}
}