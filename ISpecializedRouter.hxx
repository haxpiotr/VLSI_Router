#pragma once

#include "DataTransformer.hpp"
#include "NetSolution.hpp"

namespace in
{

class ISpecializedRouter
{
public:
    virtual ~ISpecializedRouter() = default;
	virtual DoglegSegment route(const Net& net) const = 0;
};

}