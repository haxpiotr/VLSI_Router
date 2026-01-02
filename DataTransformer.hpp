#pragma once

#include "DEFData.hpp"
#include "LEFData.hpp"

namespace in
{
	namespace bg = boost::geometry;
	using point_int = bg::model::d2::point_xy<int>;
	using box_int = bg::model::box<point_int>;
	using point = bg::model::d2::point_xy<double>;
	using box = bg::model::box<point>;
	using LibraryPinsKey = std::pair<std::string, def::Orientation>;

	struct Pin
	{
		std::pair<std::string, std::string> compIdPair;
		std::map<std::string, std::vector<box_int>> portGeometry;
	};

	box_int getPinBBox(const Pin& pin, const std::string& layer);
	box_int getPinBBox(const Pin& pin);
	point_int getPinCenter(const Pin& pin);

	struct GlobalRoutingCell
	{
		int horizontalCongestion{ 0 };
		int verticalCongestion{ 0 };
		box_int box{ {0,0},{0,0} };
	};

	class GlobalRoutingGrid
	{
	public:
		using Indices = std::vector<int>;
		using Net = std::pair<std::string, std::vector<int>>;
		using Netlist = std::vector<Net>;
		GlobalRoutingGrid() = default;
		GlobalRoutingGrid(int cols,
						  int rows,
						  const box_int& area,
						  const std::vector<def::Tracks>& tracks,
						  const std::vector<def::Net>& nets);
		[[nodiscard]] Netlist getNetlist(const std::vector<Pin>& placedPins) const;
		[[nodiscard]] size_t getCount() const;
		[[nodiscard]] std::pair<int,int> getCellCapacity() const;
		const std::vector<GlobalRoutingCell>& getGrid() const;
		[[nodiscard]] int getIndex(point_int point) const;
		[[nodiscard]] std::vector<int> getNeighbours(int i) const;
	private:
		void calculateCellCapacity();
		int m_cols{ 0 };
		int m_rows{ 0 };
		int m_count{ 0 };
		int m_xStep{ 0 };
		int m_yStep{ 0 };
		int m_firstX{ 0 };
		int m_firstY{ 0 };
		box_int m_area { {0,0},{0,0} };
		std::vector<def::Tracks> m_tracks;
		std::vector<def::Net> m_designNets;
		int m_cellHorizontalCapacity{ 0 };
		int m_cellVerticalCapacity{ 0 };
		std::vector<GlobalRoutingCell> m_grid;
	};

	class DataTransformer
	{
	public:
		DataTransformer(const lef::Data& library, const def::Data& design);
		[[nodiscard]] std::vector<Pin> getPlacedPins();
		[[nodiscard]] GlobalRoutingGrid getGlobalGrid(unsigned int cols, unsigned int rows);

	private:
		void resizeLibraryPins();
		void rotateLibraryPins();
		void placeDesignPins();

		[[nodiscard]] std::pair<int, int> getSize(const lef::Macro& macro) const;

		[[nodiscard]] std::vector<Pin> getResizedPins(const lef::Macro& macro) const;
		[[nodiscard]] std::vector<Pin> getResizedPinsUnseq(const lef::Macro& macro) const;

		[[nodiscard]] Pin getRotatedPin(const lef::Macro& macro, const Pin& pin, def::Orientation orientation) const;
		[[nodiscard]] std::vector<Pin> getRotatedPins(const lef::Macro& macro, const std::vector<Pin>& pins, def::Orientation orientation) const;

		[[nodiscard]] Pin placePin(const def::Component& macro, const Pin& pin) const;

		const std::map<LibraryPinsKey, std::vector<Pin>>& getResizedLibraryPins() const;

		[[nodiscard]] Pin getPlacedDesignPin(const def::Pin& designPin) const;
		
	private:
		
		const lef::Data& m_library;
		const def::Data& m_design;
		std::map<LibraryPinsKey, std::vector<Pin>> m_resizedPins;
		std::map<std::string, Pin> m_designPins;
		
	};

}


