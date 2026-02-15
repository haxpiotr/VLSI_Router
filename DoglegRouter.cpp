#include "DoglegRouter.hpp"

namespace in
{

std::unique_ptr<DoglegRouter> createDoglegRouter(DoglegType type)
{
	if (type == DoglegType::UPPER)
	{
		return std::make_unique<UpperDoglegRouter>();
	}
	else
	{
		return  std::make_unique<LowerDoglegRouter>();
	}
}

bool DoglegRouter::verifyForDogleg(const Net& net) const
{
	if (net.second.size() != 2)
	{
		return false;
	}

	return true;
}

DoglegSegment DoglegRouter::route(const Net& net) const
{
	if (!verifyForDogleg(net))
	{
		return {};
	}

	return perform(net.second[0], net.second[1]);
}

DoglegSegment UpperDoglegRouter::perform(const Coord& a, const Coord& b) const
{		
	auto compareVertical = [](const Coord& a, const Coord& b)
		{
			return a.y() < b.y();
		};

	auto compareHorizontal = [](const Coord& a, const Coord& b)
		{
			return a.x() < b.x();
		};

	const auto& [startV, endV] = std::minmax(a, b, compareVertical);
	const auto& [startH, endH] = std::minmax(a, b, compareHorizontal);

	DoglegSegment result;
	result.verticalSegment = { startV, {startV.x(), endV.y()} };
	result.horizontalSegment = { {startH.x(),endV.y()}, {endH.x(), endV.y()} };

	return result;
}

DoglegSegment LowerDoglegRouter::perform(const Coord& a, const Coord& b) const
{
	auto compareVertical = [](const Coord& a, const Coord& b)
		{
			return a.y() < b.y();
		};

	auto compareHorizontal = [](const Coord& a, const Coord& b)
		{
			return a.x() < b.x();
		};

	const auto& [startV, endV] = std::minmax(a, b, compareVertical);
	const auto& [startH, endH] = std::minmax(a, b, compareHorizontal);

	DoglegSegment result;
	result.verticalSegment = { {endV.x(),startV.y()}, endV };
	result.horizontalSegment = { {startH.x(),startV.y()}, { endH.x(), startV.y() } };

	return result;
}
}