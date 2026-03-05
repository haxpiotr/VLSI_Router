#pragma once

#include "DataTransformer.hpp"
#include "GeometryTypes.hpp"

#include <random>

namespace in
{
    using Net = GlobalRoutingGrid::Net;
    using Coord = GlobalRoutingGrid::Coordinates;
	using Segment = std::pair<Coord, Coord>;
    
    struct DoglegSegment
    {
        Segment verticalSegment;
		Segment horizontalSegment;
		DoglegType type;
    };

    struct NetSolution
    {
        std::string name;
        DoglegType type;
        Segment endpoints;
    };

    using GlobalSolutions = std::vector<NetSolution>;

    float getUniform(std::mt19937& gen,float from, float to);
    size_t getUniform(std::mt19937& gen,size_t from, size_t to);
    void flipDoglegType(NetSolution& solution);
    DoglegSegment route(const NetSolution& solution);
    GlobalSolutions createRandomSolutions(const GlobalSolutions& solutions, std::mt19937& gen);
    int manhattanDistance(const Segment &segment);
}