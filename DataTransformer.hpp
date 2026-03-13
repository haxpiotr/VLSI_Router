#pragma once

#include "DEFData.hpp"
#include "LEFData.hpp"

#include "GeometryTypes.hpp"

namespace in
{
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
		int congestion{ 0 };
		box_int box{ {0,0},{0,0} };
	};

	struct GlobalRoutingCells
	{
		std::vector<GlobalRoutingCell> horizontalCells;
		std::vector<GlobalRoutingCell> verticalCells;
		float penalty{ 0.0f };
	};

	class GlobalRoutingGrid
	{
	public:
		using Indices = std::vector<int>;
		using Coordinates = point_int;
		using Net = std::pair<std::string, std::vector<Coordinates>>;
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
		const GlobalRoutingCells& getGrid() const;
		[[nodiscard]] point_int getCoordinates(point_int point) const;
        [[nodiscard]] int getIndexHorizontal(point_int point) const;
        [[nodiscard]] int getIndexVertical(point_int point) const;
		[[nodiscard]] std::vector<int> getNeighbours(int i) const;
		[[nodiscard]] int getCols() const;
		[[nodiscard]] int getRows() const;
	private:
		void calculateCellCapacity();
		int m_cols{ 0 };
		int m_rows{ 0 };
		int m_count{ 0 };
		float m_xStep{ 0 };
		float m_yStep{ 0 };
		int m_firstX{ 0 };
		int m_firstY{ 0 };
		box_int m_area { {0,0},{0,0} };
		std::vector<def::Tracks> m_tracks;
		std::vector<def::Net> m_designNets;
		int m_cellHorizontalCapacity{ 0 };
		int m_cellVerticalCapacity{ 0 };
		int m_maxX{ 0 };
		int m_maxY{ 0 };
		GlobalRoutingCells m_grid;
	};

	struct SteinerTreeSegment
	{
		point_int a;
		point_int b;
		DoglegType type;
	};

	struct SteinerTreeNet
	{
		std::string name;
		std::vector<SteinerTreeSegment> segments;
	};

	struct SteinerTreeNetlist
	{
		std::vector<SteinerTreeNet> nets;
	};

	struct TreeSegment
	{
		std::pair<std::string, std::string> aName;
		std::pair<std::string, std::string> bName;
		point_int a;
		point_int b;
		int weight;
	};

	struct TreeNet
	{
		std::string name;
		std::vector<TreeSegment> segments;
	};

	struct TreeNetlist
	{
		std::vector<TreeNet> nets;
	};

	using KeyType = std::pair<std::string, std::string>;
	struct KeyComparator
	{
		bool operator()(const KeyType& a,const KeyType& b) const
		{
			auto key_comparator = [](const auto& u, const auto& v)
				{
					return u < v;
				};
			return key_comparator(a, b);
		}
	};

	class TreeTransformer
	{
	public:
		TreeTransformer(const def::Data& design, const std::vector<Pin>& placedPins);
		TreeNetlist getMST();
		SteinerTreeNetlist getRMST();
	private:
		const def::Data& m_design;
		const std::vector<Pin>& m_placedPins;
		std::map<KeyType, Pin, KeyComparator> m_pinMap;
	};

	class DataTransformer
	{
	public:
		DataTransformer(const lef::Data& library, const def::Data& design);
		const std::vector<Pin>& getPlacedPins() const;
		[[nodiscard]] GlobalRoutingGrid getGlobalGrid(unsigned int cols, unsigned int rows);
		TreeNetlist getMST();
		SteinerTreeNetlist getRMST();

	private:
		void resizeLibraryPins();
		void rotateLibraryPins();
		void placePins();

		[[nodiscard]] std::pair<int, int> getSize(const lef::Macro& macro) const;
		[[nodiscard]] int getOriginX(const lef::Macro& macro) const;
		[[nodiscard]] std::vector<Pin> getResizedPins(const lef::Macro& macro) const;
		[[nodiscard]] std::vector<Pin> getResizedPinsUnseq(const lef::Macro& macro) const;
		[[nodiscard]] Pin getRotatedPin(const lef::Macro& macro, const Pin& pin, def::Orientation orientation) const;
		[[nodiscard]] std::vector<Pin> getRotatedPins(const lef::Macro& macro, const std::vector<Pin>& pins, def::Orientation orientation) const;
		[[nodiscard]] std::vector<Pin> performPinPlacement();
		[[nodiscard]] Pin placePin(const def::Component& macro, const Pin& pin) const;
		const std::map<LibraryPinsKey, std::vector<Pin>>& getResizedLibraryPins() const;
		[[nodiscard]] Pin getPlacedDesignPin(const def::Pin& designPin) const;
		
	private:
		const lef::Data& m_library;
		const def::Data& m_design;
		std::map<LibraryPinsKey, std::vector<Pin>> m_resizedPins;
		std::map<std::string, Pin> m_designPins;
		std::vector<Pin> m_placedPins;
		std::unique_ptr<TreeTransformer> m_treeTransformer;
	};

}


