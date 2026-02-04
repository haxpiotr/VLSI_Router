#pragma once

#include "ISpecializedRouter.hxx"

#include <random>

namespace in
{
    struct NetSolution
    {
        std::string name;
        DoglegType type;
        std::vector<Coord> path;
    };

    using GlobalSolutions = std::vector<NetSolution>;

    float getUniform(std::mt19937& gen,float from, float to);
    size_t getUniform(std::mt19937& gen,size_t from, size_t to);
    void flipDoglegType(NetSolution& solution);
    void route(NetSolution& solution);
    GlobalSolutions createRandomSolutions(const GlobalSolutions& solutions, std::mt19937& gen);
}