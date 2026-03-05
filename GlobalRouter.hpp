#pragma once

#include <random>

#include "DataTransformer.hpp"
#include "ISpecializedRouter.hxx"
#include "SeqSA.hpp"
#include "ParSA.hpp"
#include "SpacePartitionedSA.hpp"
#include "SpacePartitionedGPUSA.hpp"

namespace in
{

class IGlobalRouter
{
public:
	virtual ~IGlobalRouter() = default;
};

using Indices = GlobalRoutingGrid::Indices;
using Net = GlobalRoutingGrid::Net;
using Netlist = GlobalRoutingGrid::Netlist;
using Coord = GlobalRoutingGrid::Coordinates;
using NetLeg = std::pair<std::string, std::string>;

struct GlobalRoutingData
{
	std::vector<std::string> netNames;
	std::vector<NetLeg> netLegs;
	std::vector<int> netStartsX;
	std::vector<int> netStartsY;
	std::vector<int> netEndsX;
	std::vector<int> netEndsY;
};

struct SteinerTreeResult
{
	DoglegType type{ DoglegType::ANY };
	Segment segment;
};

using CoordPair = std::pair<Coord,Coord>;

struct CoordComparator
{
	inline bool operator()(const Coord& a, const Coord& b) const
	{
		if (a.x() < b.x())
		{
			return true;
		}
		if (a.x() > b.x())
		{
			return false;
		}
		return a.y() < b.y();
	}
};

struct CoordPairComparator
{
	inline bool operator()(const CoordPair& a, const CoordPair& b) const
	{
		if (CoordComparator{}(a.first, b.first))
		{
			return true;
		}
		if (CoordComparator{}(b.first, a.first))
		{
			return false;
		}
		return CoordComparator{}(a.second, b.second);
	}
};

using SteinerTreeMap = std::map<std::pair<Coord, Coord>, DoglegType, CoordPairComparator>;

inline std::pair<Coord, Coord> closestPair(const std::vector<Coord>& pts)
{
	double best = std::numeric_limits<double>::infinity();
	std::pair<Coord, Coord> bestPair;

	for (size_t i = 0; i < pts.size(); ++i)
	{
		for (size_t j = i + 1; j < pts.size(); ++j)
		{
			double d = manhattanDistance({ pts[i], pts[j] });
			if (d < best)
			{
				best = d;
				bestPair = { pts[i],pts[j] };
			}
		}
	}

	return bestPair;
}

inline std::pair<Coord, Coord> closestExistingPair(const std::pair<Coord, Coord>& coordPair, const std::vector<Coord>& pts)
{
	double best = std::numeric_limits<double>::infinity();
	std::pair<Coord, Coord> bestPair;

	for (const auto& point : pts)
	{
		double d = manhattanDistance({ point,coordPair.first });
		if (d < best)
		{
			best = d;
			bestPair = { coordPair.first,point };
		}
	}

	for (const auto& point : pts)
	{
		double d = manhattanDistance({ point,coordPair.second });
		if (d < best)
		{
			best = d;
			bestPair = { coordPair.second,point };
		}
	}

	return bestPair;
}

inline std::pair<Coord, Coord> closestPair(const box_int& box, const std::vector<Coord>& pts)
{
	double best = std::numeric_limits<double>::infinity();
	std::pair<Coord, Coord> bestPair;

	point_int c1{ bg::get<0>(box.min_corner()), bg::get<1>(box.max_corner())};
	point_int c2{ bg::get<0>(box.max_corner()), bg::get<1>(box.min_corner()) };

	std::array<point_int, 4> corners{ box.min_corner(),box.max_corner(),c1 ,c2 };

	for (const auto& corner : corners)
	{
		for (const auto& point : pts)
		{
			double d = manhattanDistance({ point,corner });
			if (d < best)
			{
				best = d;
				bestPair = { corner,point };
			}
		}
	}

	return bestPair;
}

template <typename Point>
inline bool sameVerticalOrHorizontal(const Point& a, const Point& b)
{
	return bg::get<0>(a) == bg::get<0>(b) ||   // same x
		bg::get<1>(a) == bg::get<1>(b);    // same y
}


inline SteinerTreeMap sequentialSteinerTreeHeuristic(const std::vector<Coord>& points)
{
	namespace bg = boost::geometry;

	std::vector<Coord> pPrim = points;
	SteinerTreeMap result;

	Coord pA, pB;
	std::tie(pA,pB) = closestPair(pPrim);
	result[{pA, pB}] = DoglegType::NONE;

	pPrim.erase(std::remove_if(pPrim.begin(), pPrim.end(), [&](const auto& b) {
		return bg::equals(b, pA);
		}), pPrim.end());

	pPrim.erase(std::remove_if(pPrim.begin(), pPrim.end(), [&](const auto& b) {
		return bg::equals(b, pB);
		}), pPrim.end());

	if (pPrim.empty())
	{
		result[{pA, pB}] = DoglegType::ANY;
		return result;
	}

    box_int currentMBB{ pA,pA };
    bg::expand(currentMBB, pB);

	Coord pMBB, pC;

    while (!pPrim.empty())
    {
		//Find closest point from currentBoundingBox of pA and pB and pPrim points
		//Assign it undecided dogleg type
		//Get closest pair from existing points of MBB
		const auto clstExsPair = closestExistingPair({ pA,pB }, pPrim);
        const auto clstPair = closestPair(currentMBB, pPrim);

		//If distances are the same, go through pin
		if (manhattanDistance(clstExsPair) == manhattanDistance(clstPair) && !bg::equals(clstExsPair.first, clstPair.first))
		{
			std::tie(pMBB, pC) = clstExsPair;

			if (clstPair.first.y() < pMBB.y())
			{
				result[{pA, pB}] = DoglegType::UPPER;
			}
			else
			{
				result[{pA, pB}] = DoglegType::LOWER;
			}
		}
		else
		{
			std::tie(pMBB, pC) = clstPair;
			result[{pA, pB}] = DoglegType::NONE;
		}

		//remove pC from pPrim
        pPrim.erase(std::remove_if(pPrim.begin(), pPrim.end(), [&](const auto& b) 
		{
            return bg::equals(b, pC);
        }),
		pPrim.end());

		//If it is one of destination points then it makes no difference which dogleg is taken
        if (std::find_if(points.begin(), points.end(), [&pMBB](const auto& b) {return bg::equals(b, pMBB); }) != std::end(points))
        {
			if (result[{pMBB, pB}] == DoglegType::NONE)
			{
				result[{pMBB, pB}] = DoglegType::ANY;
			}
        } // Otherwise check if it lays on upper or lower leg of pA and pB connection
        else
        {
            NetSolution sol;
			sol.endpoints = { pA,pB };
            sol.type = DoglegType::UPPER;

            auto doglegSegment = route(sol);

			//Check if it lays on upper leg
            if (bg::equals(doglegSegment.verticalSegment.second, pMBB))
            {
				result[{pA, pB}] = DoglegType::UPPER;
            }

            sol.type = DoglegType::LOWER;
            doglegSegment = route(sol);

			//Check if it lays on lower leg
            if (bg::equals(doglegSegment.verticalSegment.first, pMBB))
            {
				result[{pA, pB}] = DoglegType::LOWER;
            }

			if (result[{pA, pB}] == DoglegType::NONE)
			{
				const auto& [tmpP1, tmpP2] = clstPair;
				NetSolution sol;
				sol.endpoints = { pA,pB };
				sol.type = DoglegType::UPPER;

				auto doglegSegment = route(sol);

				//Check if it lays on upper leg
				if (bg::equals(doglegSegment.verticalSegment.second, tmpP1))
				{
					result[{pA, pB}] = DoglegType::LOWER;
				}

				sol.type = DoglegType::LOWER;
				doglegSegment = route(sol);

				//Check if it lays on lower leg
				if (bg::equals(doglegSegment.verticalSegment.first, tmpP1))
				{
					result[{pA, pB}] = DoglegType::UPPER;
				}
			}

        }
        box_int newMBB{ pMBB,pMBB };
        bg::expand(newMBB, pC);
		pA = pMBB;
		pB = pC;
        currentMBB = newMBB;
    }

	for (auto& [key, res] : result)
	{
		if (res != DoglegType::NONE)
		{
			continue;
		}

		if (sameVerticalOrHorizontal(key.first, key.second))
		{
			res = DoglegType::ANY;
		}
	}

	result[{ pMBB, pC }] = DoglegType::ANY;
	

	for (auto it = result.begin(); it != result.end(); )
	{
		const auto& key = it->first;

		if (boost::geometry::equals(key.first, key.second))
		{
			it = result.erase(it);   // safe erase
		}
		else
		{
			++it;
		}
	}


    return result;
}

class GlobalRouter : public IGlobalRouter
{
public:
	
	GlobalRouter(DataTransformer& dataTransformer, unsigned int cols, unsigned int rows);
	~GlobalRouter() = default;
	void createInitialSolution();

	const Netlist& getNetlist() const;
	std::map<int, int> getNetlistElementHistogram() const;
	const GlobalSolutions& getDoglegSolutions() const;
	const GlobalRoutingCells& getGrid() const;

	void performSA(size_t maxIterations, float initialTemperature, float coolingRate);
	void performSAPar(size_t maxIterations, float initialTemperature, float coolingRate);
	void performSAParSpacePartitioned(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces);
	void performSAParSpacePartitionedOnGPU(size_t maxIterations, float initialTemperature, float coolingRate, size_t spaces);
private:
	DataTransformer& m_dataTransformer;
	std::vector<Pin> m_placedPins;
	GlobalRoutingGrid m_globalGrid;
	GlobalRoutingCells m_grid;
	std::vector<DoglegType> m_doglegTypes;
	Netlist m_netlist;
	Netlist m_twoPointNets;
	GlobalSolutions m_doglegSolutions;
	std::unique_ptr<IOptimizationSolver> m_solver;

	void readAllTwoPointNets();
	void initializeDoglegTypes();
	void readAllDoglegNets();
	void placeNetsOnGrid();
};

}