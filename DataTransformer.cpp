#include "DataTransformer.hpp"

#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

#include <execution>
#include <fstream>
#include <omp.h>

#include "MinimumSpanningTree.hpp"
#include "NetSolution.hpp"

namespace in
{
	namespace bg = boost::geometry;

	box_int getPinBBox(const Pin& pin, const std::string& layer)
	{
		const auto& geo = pin.portGeometry.at(layer);
		box_int acc;
		bg::assign_inverse(acc);

		for (auto const& g : geo) 
		{
			bg::expand(acc, g);            
		}

		return acc;
	}

	box_int getPinBBox(const Pin& pin)
	{
		box_int acc;
		bg::assign_inverse(acc);

		for (const auto& [layer, _] : pin.portGeometry)
		{
			bg::expand(acc, getPinBBox(pin, layer));
		}

		return acc;
	}

	point_int getPinCenter(const Pin& pin)
	{
		const auto box = getPinBBox(pin);
		point_int result;
		bg::centroid(box, result);
		return result;
	}


	TreeTransformer::TreeTransformer(const def::Data& design, const std::vector<Pin>& placedPins)
		: m_design{design}, m_placedPins{placedPins}
	{
		for (const auto& placedPin : m_placedPins)
		{
			m_pinMap[placedPin.compIdPair] = placedPin;
		}
	}

	TreeNetlist TreeTransformer::getMST()
	{
		auto key_comparator = [](const auto& a, const auto& b)
			{
				return a.compIdPair < b;
			};


		std::vector<std::vector<TreeNet>> localNetlists(omp_get_max_threads());

#pragma omp parallel for 
		for (size_t i = 0; i < m_design.nets.size(); ++i)
		{
			std::vector<in::point_int> points;
			std::vector<std::pair<std::string, std::string>> compIdPairs;
			for (const auto& key : m_design.nets[i].compPinPairs)
			{
				const auto& pin = m_pinMap[key];
				const auto pinCenter = getPinCenter(pin);
				points.push_back(pinCenter);
				compIdPairs.push_back(pin.compIdPair);

			}
			const auto mst = tree::rectilinearMST(points);
			
			TreeNet tNet;
			tNet.name = m_design.nets[i].name;

			for (const auto& e : mst)
			{
				tNet.segments.push_back(TreeSegment{ compIdPairs[e.u], compIdPairs[e.v], points[e.u], points[e.v], e.weight});
			}

			localNetlists[omp_get_thread_num()].push_back(tNet);
		}

		size_t totalSize{ 0 };
		for (const auto& v : localNetlists)
		{
			totalSize += v.size();
		}

		TreeNetlist tNetlist;
		tNetlist.nets.reserve(totalSize);

		for (const auto& v : localNetlists)
		{
			tNetlist.nets.insert(tNetlist.nets.end(), v.begin(), v.end());
		}

		return tNetlist;
	}

	SteinerTreeNetlist TreeTransformer::getRMST()
	{
		auto key_comparator = [](const auto& a, const auto& b)
			{
				return a.compIdPair < b;
			};

		std::vector<std::vector<SteinerTreeNet>> localNets(omp_get_max_threads());

#pragma omp parallel for 
		for (size_t i = 0; i < m_design.nets.size(); ++i)
		{
			std::vector<in::point_int> points;
			for (const auto& key : m_design.nets[i].compPinPairs)
			{
				const auto& pin = m_pinMap[key];
				const auto pinCenter = getPinCenter(pin);
				points.push_back(pinCenter);
			}
			const auto rmstResult = tree::rectinilearSteinerMST(points);
			const auto& [rmst, resultPoints] = rmstResult;

			SteinerTreeNet steinerNet;
			steinerNet.name = m_design.nets[i].name;

			for (const auto& e : rmst)
			{
				SteinerTreeSegment segment;
				segment.type = DoglegType::ANY;
				segment.a = { resultPoints[e.u].x(),resultPoints[e.u].y() };
				segment.b = { resultPoints[e.v].x(),resultPoints[e.v].y() };
				if (segment.a.x() == segment.b.x() || segment.a.y() == segment.b.y())
				{
					segment.type = DoglegType::LINE;
				}
				steinerNet.segments.push_back(segment);
			}

			localNets[omp_get_thread_num()].push_back(steinerNet);
		}

		size_t totalSize{ 0 };
		for (const auto& v : localNets)
		{
			totalSize += v.size();
		}

		SteinerTreeNetlist steinerNetlist;
		steinerNetlist.nets.reserve(totalSize);

		for (const auto& v : localNets)
		{
			steinerNetlist.nets.insert(steinerNetlist.nets.end(), v.begin(), v.end());
		}

		return steinerNetlist;
	}

	DataTransformer::DataTransformer(const lef::Data& library, const def::Data& design) :
		m_library{ library }, m_design{ design } 
	{
		resizeLibraryPins();
		rotateLibraryPins();
		placePins();
		m_treeTransformer = std::make_unique<TreeTransformer>(design, m_placedPins);
	};

	Pin DataTransformer::getPlacedDesignPin(const def::Pin& designPin) const
	{
		const auto orientation = designPin.orientation;

		Pin placedPin;
		const std::string placedPinName {"PIN"};
		placedPin.compIdPair = { placedPinName, designPin.name };

		switch (orientation)
		{
			case def::Orientation::N:
			case def::Orientation::FN:
			case def::Orientation::S:
			case def::Orientation::FS:
			{
				box_int box = { {designPin.bounds.min_corner().x() + designPin.placementX,designPin.bounds.min_corner().y() + designPin.placementY},
					{designPin.bounds.max_corner().x() + designPin.placementX,designPin.bounds.max_corner().y() + designPin.placementY} };
				std::vector<box_int> boxVec;
				boxVec.push_back(std::move(box));
				placedPin.portGeometry[designPin.layer] = boxVec;

				return placedPin;
			}
			default:
			{
				box_int box = { {designPin.bounds.min_corner().x() + designPin.placementX,
								 designPin.bounds.min_corner().y() + designPin.placementY},
					{designPin.bounds.min_corner().x() + (designPin.bounds.max_corner().y() - designPin.bounds.min_corner().y()) + designPin.placementX,
					 designPin.bounds.min_corner().y() + (designPin.bounds.max_corner().x() - designPin.bounds.min_corner().x()) + designPin.placementY}};
				std::vector<box_int> boxVec;
				boxVec.push_back(std::move(box));
				placedPin.portGeometry[designPin.layer] = boxVec;

				return placedPin;
			}
		}
	}

	const std::vector<Pin>& DataTransformer::getPlacedPins() const
	{
		return m_placedPins;
	}

	std::vector<Pin> DataTransformer::performPinPlacement()
	{
		std::vector<std::vector<Pin>> localPins(omp_get_max_threads());

		const auto& libraryPins = getResizedLibraryPins();

#pragma omp parallel for
		for (size_t i = 0; i< m_design.components.size(); ++i)
		{
			const auto& libPins = libraryPins.at({ m_design.components[i].name,m_design.components[i].orientation});

			for (const auto& libPin : libPins)
			{
				localPins[omp_get_thread_num()].push_back(placePin(m_design.components[i], libPin));
			}
		}

#pragma omp parallel for
		for (size_t i = 0; i < m_design.pins.size(); ++i)
		{
			localPins[omp_get_thread_num()].push_back(getPlacedDesignPin(m_design.pins[i]));
		}

		size_t totalSize{ 0 };
		for (const auto& v : localPins)
		{
			totalSize += v.size();
		}

		std::vector<Pin> pins;
		pins.reserve(totalSize);
		
		for (const auto& v : localPins)
		{
			pins.insert(pins.end(), v.begin(), v.end());
		}

		return pins;
	}

	void DataTransformer::placePins()
	{
		m_placedPins = performPinPlacement();
		auto key_comparator = [](const auto& a, const auto& b)
			{
				return a.compIdPair < b.compIdPair;
			};
		std::sort(std::execution::par, m_placedPins.begin(), m_placedPins.end(), key_comparator);
	}

	GlobalRoutingGrid DataTransformer::getGlobalGrid(unsigned int cols, unsigned int rows)
	{
		return GlobalRoutingGrid(cols, rows, m_design.dieArea, m_design.tracks, m_design.nets);
	}

	TreeNetlist DataTransformer::getMST()
	{
		return m_treeTransformer->getMST();
	}

	SteinerTreeNetlist DataTransformer::getRMST()
	{
		return m_treeTransformer->getRMST();
	}

	std::pair<int, int> DataTransformer::getSize(const lef::Macro& macro) const
	{
		const auto dbUnits = m_library.dbUnits;
		const int macroSizeX = static_cast<int>(macro.sizeX * dbUnits);
		const int macroSizeY = static_cast<int>(macro.sizeY * dbUnits);

		return { macroSizeX ,macroSizeY };
	}

	int DataTransformer::getOriginX(const lef::Macro& macro) const
	{
		const auto dbUnits = m_library.dbUnits;
		const int macroOriginX= static_cast<int>(macro.originX * dbUnits);

		return macroOriginX;
	}

	std::vector<Pin> DataTransformer::getRotatedPins(const lef::Macro& macro,const std::vector<Pin>& pins, def::Orientation orientation) const
	{
		std::vector<Pin> result;
		result.reserve(pins.size() * 7);
		
		for (const auto& pin : pins)
		{
			result.push_back(getRotatedPin(macro, pin, orientation));
		}

		return result;
	}

	std::vector<Pin> DataTransformer::getResizedPins(const lef::Macro& macro) const
	{
		const auto dbUnits = m_library.dbUnits;

		std::vector<Pin> pins;
		pins.reserve(macro.pins.size());

		for (const auto& libPin : macro.pins)
		{
			Pin pin;
			pin.compIdPair.second = libPin.name;

			for (const auto& [layer, geo] : libPin.pinGeometry)
			{
				std::vector<box_int> boxes;
				boxes.resize(geo.size());
				std::transform(geo.cbegin(), geo.cend(), std::back_inserter(boxes),
					[&dbUnits](const auto& e) -> in::box_int
					{
						return { {static_cast<int>(e.min_corner().x() * dbUnits), static_cast<int>(e.min_corner().y() * dbUnits)},
								 {static_cast<int>(e.max_corner().x() * dbUnits), static_cast<int>(e.max_corner().y() * dbUnits)} };
					});
				pin.portGeometry[layer] = std::move(boxes);
			}

			pins.push_back(std::move(pin));
		}

		return pins;
	}

	std::vector<Pin> DataTransformer::getResizedPinsUnseq(const lef::Macro& macro) const
	{
		const auto dbUnits = m_library.dbUnits;

		std::vector<Pin> pins;
		pins.reserve(macro.pins.size());

		for (const auto& libPin : macro.pins)
		{
			Pin pin;
			pin.compIdPair.second = libPin.name;

			for (const auto& [layer, geo] : libPin.pinGeometry)
			{
				std::vector<box_int> boxes;
				boxes.resize(geo.size());
				std::transform(std::execution::unseq, geo.cbegin(), geo.cend(), boxes.begin(),
					[&dbUnits](const auto& e) -> in::box_int
					{
						return { {static_cast<int>(e.min_corner().x() * dbUnits), static_cast<int>(e.min_corner().y() * dbUnits)},
								 {static_cast<int>(e.max_corner().x() * dbUnits), static_cast<int>(e.max_corner().y() * dbUnits)} };
					});
				pin.portGeometry[layer] = std::move(boxes);
			}

			pins.push_back(std::move(pin));
		}

		return pins;
	}

	void DataTransformer::resizeLibraryPins()
	{
		for (const auto& macro : m_library.macros)
		{
			m_resizedPins[{macro.name, def::Orientation::N}] = getResizedPinsUnseq(macro);
		}
	}

	Pin DataTransformer::getRotatedPin(const lef::Macro& macro, const Pin& pin, def::Orientation orientation) const
	{
		const auto& [sizeX, sizeY] = getSize(macro);

		Pin result;
		result.compIdPair = pin.compIdPair;

		if (orientation == def::Orientation::N)
		{
			return pin;
		}

		if (orientation == def::Orientation::FN)
		{
			result.portGeometry = pin.portGeometry;
		}

		if (orientation == def::Orientation::E || orientation == def::Orientation::FE)
		{
			for (const auto& [layer, geo] : pin.portGeometry)
			{
				std::vector<box_int> rotatedBox;
				rotatedBox.resize(geo.size());
				std::transform(std::execution::unseq,geo.begin(), geo.end(), rotatedBox.begin(),
					[sizeX](const auto& in) -> box_int
					{
						return { {in.min_corner().y(), -in.max_corner().x() + sizeX},
								 {in.max_corner().y(), -in.min_corner().x() + sizeX} };
					});
				result.portGeometry[layer] = std::move(rotatedBox);
			}
		}

		if (orientation == def::Orientation::S || orientation == def::Orientation::FS)
		{
			for (const auto& [layer, geo] : pin.portGeometry)
			{
				std::vector<box_int> rotatedBox;
				rotatedBox.resize(geo.size());
				std::transform(std::execution::unseq,geo.begin(), geo.end(), rotatedBox.begin(),
					[sizeX, sizeY](const auto& in) -> box_int
					{
						return { {-in.max_corner().x() + sizeX, -in.max_corner().y() + sizeY},
							     {-in.min_corner().x() + sizeX, -in.min_corner().y() + sizeY} };
					});
				result.portGeometry[layer] = std::move(rotatedBox);
			}
		}

		if (orientation == def::Orientation::W || orientation == def::Orientation::FW)
		{
			for (const auto& [layer, geo] : pin.portGeometry)
			{
				std::vector<box_int> rotatedBox;
				rotatedBox.resize(geo.size());
				std::transform(std::execution::unseq,geo.begin(), geo.end(), rotatedBox.begin(), [sizeX](const auto& in) -> box_int
					{
						return { {-in.max_corner().y() + sizeX, in.min_corner().x()},
								 {-in.min_corner().y() + sizeX, in.max_corner().x()} };
					});
				result.portGeometry[layer] = std::move(rotatedBox);
			}
		}

		if (orientation == def::Orientation::FE ||
			orientation == def::Orientation::FW ||
			orientation == def::Orientation::FN ||
			orientation == def::Orientation::FS)
		{
			const auto macroX = getOriginX(macro);
			for (auto& [_, geo] : result.portGeometry)
			{
				std::transform(std::execution::unseq,geo.begin(), geo.end(), geo.begin(), [macroX,sizeX](const auto& in) -> box_int
					{
						return { { 2 * macroX + sizeX -in.max_corner().x(), in.min_corner().y()},
								 { 2 * macroX + sizeX - in.min_corner().x(), in.max_corner().y()} };
					});
			}
		}

		return result;
	}

	void DataTransformer::rotateLibraryPins()
	{
		for (const auto& macro : m_library.macros)
		{
			const auto& regularPins = m_resizedPins[{macro.name, def::Orientation::N}];
			m_resizedPins[{macro.name, def::Orientation::E}] = getRotatedPins(macro, regularPins, def::Orientation::E);
			m_resizedPins[{macro.name, def::Orientation::S}] = getRotatedPins(macro, regularPins, def::Orientation::S);
			m_resizedPins[{macro.name, def::Orientation::W}] = getRotatedPins(macro, regularPins, def::Orientation::W);
			m_resizedPins[{macro.name, def::Orientation::FN}] = getRotatedPins(macro, regularPins, def::Orientation::FN);
			m_resizedPins[{macro.name, def::Orientation::FE}] = getRotatedPins(macro, regularPins, def::Orientation::FE);
			m_resizedPins[{macro.name, def::Orientation::FS}] = getRotatedPins(macro, regularPins, def::Orientation::FS);
			m_resizedPins[{macro.name, def::Orientation::FW}] = getRotatedPins(macro, regularPins, def::Orientation::FW);
		}
	}

	const std::map<LibraryPinsKey, std::vector<Pin>>& DataTransformer::getResizedLibraryPins() const
	{
		return m_resizedPins;
	}

	Pin DataTransformer::placePin(const def::Component& comp, const Pin& pin) const
	{
		Pin result = pin;
		result.compIdPair = pin.compIdPair;
		result.compIdPair.first = comp.id;

		const auto dX = comp.placementX;
		const auto dY = comp.placementY;

		result.portGeometry = pin.portGeometry;

		for (auto& [_,geo] : result.portGeometry)
		{
			std::transform(std::execution::unseq, geo.begin(), geo.end(), geo.begin(),
				[&dX, &dY]
				(const auto& rect) -> box_int
				{
					return { {rect.min_corner().x() + dX, rect.min_corner().y() + dY},
							{rect.max_corner().x() + dX, rect.max_corner().y() + dY} };
				});
		}

		return result;
	}

	GlobalRoutingGrid::GlobalRoutingGrid(int cols,
										 int rows,
										 const box_int& area,
										 const std::vector<def::Tracks>& tracks,
										 const std::vector<def::Net>& nets)
		: m_cols{ cols }, m_rows{ rows }, m_count{ m_cols * m_rows }, m_area{area
	}, m_tracks{ tracks }, m_designNets{ nets }
	{
		m_grid.horizontalCells.resize(m_count);
		m_grid.verticalCells.resize(m_count);

		m_xStep = static_cast<float>(m_area.max_corner().x() - m_area.min_corner().x())/m_cols;
		m_yStep = static_cast<float>(m_area.max_corner().y() - m_area.min_corner().y())/m_rows;
		m_firstX = m_area.min_corner().x();
		m_firstY = m_area.min_corner().y();
		m_maxX = m_cols - 1;
		m_maxY = m_rows - 1;

		auto getBox = [this](int i) -> box_int
			{
				const int scaledI = i % m_cols;
				const int scaledJ = i / m_cols;
				return { { m_firstX + static_cast<int>(scaledI * m_xStep), m_firstY + static_cast<int>(scaledJ * m_yStep)},
						 { m_firstX + static_cast<int>((scaledI + 1) * m_xStep), m_firstY + static_cast<int>((scaledJ + 1) * m_yStep) } };
			};

		for (int i = 0; i < m_count; ++i)
		{ 
			m_grid.horizontalCells[i].box = getBox(i);
            m_grid.verticalCells[i].box = getBox(i);
		}

		calculateCellCapacity();
	}

	GlobalRoutingGrid::Netlist GlobalRoutingGrid::getNetlist(const std::vector<Pin>& placedPins) const
	{
		auto key_comparator = [](const auto& a, const auto& b)
			{
				return a.compIdPair < b;
			};

		GlobalRoutingGrid::Netlist result;
		result.reserve(m_designNets.size());
		result.resize(m_designNets.size());

		for (size_t i = 0; i < m_designNets.size(); i++)
		{
			result[i] = {};
		}

		for (size_t i = 0; i < result.size(); ++i)
		{
			const auto& designNet = m_designNets[i];
			Net net;
			net.first = designNet.name;
			net.second.reserve(designNet.compPinPairs.size());

			for (const auto& key : designNet.compPinPairs)
			{
				auto pinIt = std::lower_bound(placedPins.begin(),
					placedPins.end(), key, key_comparator);

				const auto& pin = *pinIt;
				const auto pinCenter = getPinCenter(pin);
				const auto pinIndex = getCoordinates(pinCenter);
				net.second.emplace_back(pinIndex);
			}
			result[i] = std::move(net);
		}

		return result;
	}

	void GlobalRoutingGrid::calculateCellCapacity()
	{
		const auto standardBox = m_grid.horizontalCells.front().box;

		int horizontal{ 0 };
		int vertical{ 0 };

		const int xSpan = standardBox.max_corner().x() - standardBox.min_corner().x();
		const int ySpan = standardBox.max_corner().y() - standardBox.min_corner().y();

		for (const auto& track : m_tracks)
		{
			if (track.macro == def::Tracks::Macro::X)
			{
				horizontal += xSpan / static_cast<int>(track.step);
			}
			else
			{
				vertical += ySpan / static_cast<int>(track.step);
			}
		}

		m_cellHorizontalCapacity = horizontal;
		m_cellVerticalCapacity = vertical;
	}

	std::pair<int, int> GlobalRoutingGrid::getCellCapacity() const
	{
		return { m_cellHorizontalCapacity, m_cellVerticalCapacity };
	}

	const GlobalRoutingCells& GlobalRoutingGrid::getGrid() const
	{
		return m_grid;
	}

	point_int GlobalRoutingGrid::getCoordinates(point_int point) const
	{
		const auto xI = std::min(m_maxX, static_cast<int>(static_cast<float>(point.x() - m_firstX) / m_xStep));
		const auto yI = std::min(m_maxY, static_cast<int>(static_cast<float>(point.y() - m_firstY) / m_yStep));

		return {xI, yI};
	}

	int GlobalRoutingGrid::getIndexHorizontal(point_int point) const
	{
		const auto xI = point.x();
		const auto yI = point.y();

		return yI * m_cols + xI;
	}

	int GlobalRoutingGrid::getIndexVertical(point_int point) const
    {
        const auto xI = point.x();
        const auto yI = point.y();

        return xI * m_rows + yI;
    }

	std::vector<int> GlobalRoutingGrid::getNeighbours(int i) const
	{
		std::vector<int> result;
		result.reserve(4);

		const auto xI = i % m_cols;
		const auto yI = i / m_cols;

		if (xI - 1 > 0)
		{
			result.emplace_back(i - 1);
		}
		if (xI + 1 < m_cols)
		{
			result.emplace_back(i + 1);
		}
		if (yI - 1 > 0)
		{
			result.emplace_back(i - m_cols);
		}
		if (yI + 1 < m_rows)
		{
			result.emplace_back(i + m_cols);
		}

		return result;
	}

	size_t GlobalRoutingGrid::getCount() const
	{
		return m_grid.horizontalCells.size();
	}

	int GlobalRoutingGrid::getCols() const
	{
		return m_cols;
	}

	int GlobalRoutingGrid::getRows() const
	{
		return m_rows;
	}


}