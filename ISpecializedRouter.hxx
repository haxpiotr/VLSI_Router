#pragma once

#include "DataTransformer.hpp"

namespace in
{

using RoutedPath = std::vector<point_int>;
using Net = GlobalRoutingGrid::Net;
using Coord = GlobalRoutingGrid::Coordinates;

enum class DoglegType
{
	UPPER,
	LOWER
};

class ISpecializedRouter
{
public:
    virtual ~ISpecializedRouter() = default;
	virtual RoutedPath route(const Net& net) const = 0;
};

}