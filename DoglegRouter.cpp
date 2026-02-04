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

RoutedPath DoglegRouter::route(const Net& net) const
{
	if (!verifyForDogleg(net))
	{
		return {};
	}

	return perform(net.second[0], net.second[1]);
}

std::vector<Coord> UpperDoglegRouter::perform(const Coord& a, const Coord& b) const
{		
	auto compare = [](const Coord& a, const Coord& b)
		{
			return a.y() < b.y();
		};
	const auto& [start, end] = std::minmax(a, b, compare);
	const auto verticalDistance = end.y() - start.y();
	const auto horizontalDistance = end.x() - start.x();

	const int verticalStep {1};
	const int horizontalStep = horizontalDistance > 0 ? 1 : -1;

	const size_t pathSize = 1 + std::abs(verticalDistance) + std::abs(horizontalDistance);

	std::vector<Coord> path(pathSize);

	auto it = std::generate_n(path.begin(), std::abs(verticalDistance), [&,currentY = start.y()]() mutable
	{
		Coord coord{ start.x(), currentY};
		currentY += verticalStep;
		return coord;
	});

	std::generate(it, path.end(), [&,currentX = start.x()]() mutable
	{
		Coord coord{ currentX, start.y() + verticalDistance };
		currentX += horizontalStep;
		return coord;
	});

	return path;
}

std::vector<Coord> LowerDoglegRouter::perform(const Coord& a, const Coord& b) const
{
	auto compare = [](const Coord& a, const Coord& b)
		{
			return a.y() < b.y();
		};
	const auto& [start, end] = std::minmax(a, b, compare);
	const auto verticalDistance = end.y() - start.y();
	const auto horizontalDistance = end.x() - start.x();

	const int verticalStep {1};
	const int horizontalStep = horizontalDistance > 0 ? 1 : -1;

	const size_t pathSize = 1 + std::abs(verticalDistance) + std::abs(horizontalDistance);

	std::vector<Coord> path(pathSize);

	auto it = std::generate_n(path.begin(), std::abs(horizontalDistance), [&,currentX = start.x()]() mutable
	{
		Coord coord{ currentX, start.y() };
		currentX += horizontalStep;
		return coord;
	});

	std::generate(it, path.end(), [&,currentX = start.x() + horizontalDistance, currentY = start.y()]() mutable
	{
		Coord coord{ currentX, currentY};
		currentY += verticalStep;
		return coord;
	});

	return path;
}
}