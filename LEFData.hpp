#pragma once

#include "lef/lefrReader.hpp"

#include <boost/parser/parser.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <vector>
#include <string>
#include <map>

namespace in::lef
{
	namespace bg = boost::geometry;
	using point = bg::model::d2::point_xy<double>;
	using box = bg::model::box<point>;

	struct Site
	{
		enum class Class
		{
			CORE,
			PAD
		};
		std::string name;
		Class siteClass{ Class::CORE };
		double sizeX{ 0 };
		double sizeY{ 0 };
	};

	struct Spacing
	{
		double value{ 0 };
		double eolWidth{ 0 };
		double eolWithin{ 0 };
		double parSpace{ 0 };
		double parWithin{ 0 };
		double adjCuts{ 0 };
		double adjWithin{ 0 };
	};

	struct SpacingEntry
	{
		std::vector<double> prlLengths;
		std::vector<double> prlWidths;
		std::vector<std::vector<double>> prlWidthSpacings;
	};

	struct PropertyCornerSpacing
	{
		double exceptEol{ 0 };
		std::vector<std::pair<double, double>> widthsAndSpacings;
	};

	inline std::optional<PropertyCornerSpacing> parsePropertyCornerSpacing(std::string_view s)
	{
		namespace bp = boost::parser;

		auto const ws_item =
			bp::lit("WIDTH") >> bp::double_ >>
			bp::lit("SPACING") >> bp::double_;

		[[maybe_unused]]auto const convex =
			bp::lit("CONVEXCORNER");

		auto const grammar =
			bp::lit("CORNERSPACING") >>
			bp::lit("CONVEXCORNER") >>
			bp::lit("EXCEPTEOL") >> bp::double_ >>
			+ws_item >>
			bp::char_(';');

		if (auto attr = bp::parse(s, grammar, bp::ws)) 
		{
			PropertyCornerSpacing out;
			out.exceptEol = std::get<0>(*attr);
			for (auto const& t : std::get<1>(*attr)) 
			{
				out.widthsAndSpacings.push_back(std::pair <double, double> { std::get<0>(t), std::get<1>(t) });
			}
			return out;
		}
		return std::nullopt;
	}

	struct Layer
	{
		enum class Type : std::uint8_t
		{
			ROUTING,
			CUT
		};

		enum class Direction : std::uint8_t
		{
			HORIZONTAL,
			VERTICAL
		};

		std::string name;
		Type type{ Type::CUT };
		Direction direction{ Direction::HORIZONTAL };
		std::pair<double, double> pitch{ 0,0 };
		double width{ 0 };
		double minwidth{ 0 };
		double area{ 0 };
		std::vector<Spacing> spacings;
		std::vector<SpacingEntry> spacingTable;
		std::string property;
		std::optional<PropertyCornerSpacing> prCornerSpacing;
	};

	struct Via
	{
		using BoxList = std::vector<box>;
		using ViaGeometry = std::map<std::string, BoxList>;

		std::string name;
		bool isDeafult{ false };
		ViaGeometry geometry;
	};

	struct Pin
	{
		enum class Direction : std::uint8_t
		{
			INPUT,
			OUTPUT,
			INOUT
		};

		enum class Shape : std::uint8_t
		{
			NONE,
			ABUTMENT,
			RING,
			FEEDTHRU
		};

		enum class Use : std::uint8_t
		{
			ANALOG,
			GROUND,
			POWER,
			SIGNAL,
			CLOCK
		};

		using BoxList = std::vector<box>;
		using PinGeometry = std::map<std::string, BoxList>;

		std::string name;
		Direction direction{ Direction::INPUT };
		Shape shape{ Shape::NONE };
		Use use{ Use::SIGNAL };
		PinGeometry pinGeometry;
	};

	struct Obs
	{
		using BoxList = std::vector<box>;
		using ObsGeometry = std::map<std::string, BoxList>;
		ObsGeometry obsGeometry;
	};

	struct Macro
	{
		std::string name;
		Site::Class macroClass{ Site::Class::CORE };
		double originX{ 0 };
		double originY{ 0 };
		double sizeX{ 0 };
		double sizeY{ 0 };
		std::string siteName;
		bool hasXSymmetry{ false };
		bool hasYSymmetry{ false };
		bool has90Symmetry{ false };
		std::vector<Pin> pins;
		Obs obstruction;
	};

	struct Data
	{
		enum class ClearanceMeasure : std::uint8_t
		{
			EUCLIDEAN,
			MAXXY
		};
		enum class UseMinSpacing : std::uint8_t
		{
			ON,
			OFF
		};

		double version{ 0 };
		std::string busbitChars;
		std::string dividerChar;
		double dbUnits{ 0 };
		double manufacturingGrid{ 0 };
		ClearanceMeasure clearanceMeasure{ ClearanceMeasure::MAXXY };
		UseMinSpacing useMinSpacing{ UseMinSpacing::OFF };
		std::vector<Site> sites;
		std::vector<Layer> layers;
		std::vector<Via> vias;
		std::vector<Pin> pins;
		Obs obstruction;
		std::vector<Macro> macros;
	};

}