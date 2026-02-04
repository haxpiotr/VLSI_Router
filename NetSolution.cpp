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

	GlobalSolutions createRandomSolutions(
		const GlobalSolutions& solutions,
		std::mt19937& gen)
	{
		auto randoms = solutions;
        
        std::uniform_int_distribution<int> dist(0, 1);

        for (auto& solution : randoms)
        {
            solution.type = (dist(gen) == 0) ? DoglegType::UPPER : DoglegType::LOWER;
            route(solution);
        }

        return randoms;
	}
	

	void route(NetSolution& solution)
	{
		solution.path = createDoglegRouter(solution.type)->route(
			{ solution.name, {solution.path.front(), solution.path.back()} });
	}
}