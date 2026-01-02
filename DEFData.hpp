#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <vector>
#include <string>

namespace in::def
{
	namespace bg = boost::geometry;
	using point_int = bg::model::d2::point_xy<int>;
	using box_int = bg::model::box<point_int>;

	struct Tracks
	{
		enum class Macro
		{
			X,
			Y
		};

		Macro macro{ Macro::X };
		double start{ 0 };
		double number{ 0 };
		double step{ 0 };
		std::string layer;
	};

	struct GCellGrid
	{
		enum class Macro
		{
			X,
			Y
		};

		Macro macro{ Macro::X };
		double start{ 0 };
		double number{ 0 };
		double step{ 0 };
	};

	struct Via
	{
		std::string name;
		std::map<std::string, std::vector<box_int>> viaGeometry;
		std::string viaRule;
		int cutSizeX{ 0 };
		int cutSizeY{ 0 };
		std::string bottomLayer;
		std::string cutLayer;
		std::string topLayer;
		int cutSpacingX{ 0 };
		int cutSpacingY{ 0 };
		int botEnclosureX{ 0 };
		int botEnclosureY{ 0 };
		int topEnclosureX{ 0 };
		int topEnclosureY{ 0 };
		int numCutRows{ 0 };
		int numCutCols{ 0 };
		int originX{ 0 };
		int originY{ 0 };
		int botOffX{ 0 };
		int botOffY{ 0 };
		int topOffX{ 0 };
		int topOffY{ 0 };
		std::string pattern;
	};

	enum class Orientation
	{
		N,
		E,
		S,
		W,
		FN,
		FE,
		FS,
		FW
	};

	struct Component
	{
		std::string id;
		std::string name;
		int placementX{ 0 };
		int placementY{ 0 };
		Orientation orientation{ Orientation::N };
	};

	struct Net
	{
		std::string name;
		std::vector<std::pair<std::string, std::string>> compPinPairs;
	};

	struct Pin
	{
		std::string net;
		std::string name;
		std::string layer;
		int placementX{ 0 };
		int placementY{ 0 };
		box_int bounds{ {0,0},{0,0} };
		Orientation orientation{ Orientation::N };
	};

	struct Data
	{
		box_int dieArea{ {0,0},{0,0} };
		std::vector<Tracks> tracks;
		std::vector<GCellGrid> grids;
		int viaCount{ 0 };
		std::vector<Via> vias;
		int specialNetsCount{ 0 };
		int componentsCount{ 0 };
		std::vector<Component> components;
		std::vector<Net> nets;
		std::vector<Pin> pins;
	};
}