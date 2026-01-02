#pragma once

#include <string>
#include <utility>
#include <vector>
#include <optional>

namespace lef
{
	enum class ClearanceMeasure
	{
		EUCLIDEAN,
		MAXXY
	};

	enum class UseMinSpacingObs
	{
		ON,
		OFF
	};

	struct Site
	{
		enum class Class
		{
			PAD,
			CORE
		};

		std::string name;
		Class siteClass;
		double width{ 0 };
		double height{ 0 };
	};

	struct Layer
	{
		enum class Type
		{
			ROUTING,
			CUT
		};

		enum class Direction
		{
			VERTICAL,
			HORIZONTAL
		};

		std::string name;
		Type type;
		std::optional<Direction> direction;
		std::optional<std::pair<double, double>> pitch;
		std::optional<double> width;
		std::optional<double> minWidth;
		std::optional<double> area;
		std::optional<double> parallelLengthRun;
		std::optional<std::vector<std::pair<double, double>>> spacingTable;
		std::optional<double> spacing;
		std::optional<double> endofline;
		std::optional<double> within;
	};

	struct LEFDocument
	{
		std::string version;
		char dividerChar{ 'x' };
		std::pair<char, char> busbitchars;
		double manufacturingGrid{ 0 };
		ClearanceMeasure clearanceMeasure;
		unsigned int databaseUnit = 0;
		UseMinSpacingObs useMinSpacingObs;
		std::vector<Site> sites;
		std::vector<Layer> layers;
	};
}