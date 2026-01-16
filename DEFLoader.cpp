#include "DEFLoader.hpp"

#include <fstream>
#include <cstdio>

namespace in::def
{
	Loader::~Loader()
	{
		LefDefParser::defrClear();
		m_data = Data{};
	}

	Data Loader::get(const std::filesystem::path& def)
	{
		initialize();

		if (!verify(def))
		{
			const std::string message = "Could not open: " + def.string();

			throw std::runtime_error(message);
		}

		std::FILE* defPtr = std::fopen(def.string().c_str(), "r");

		if (defPtr == nullptr)
		{
			const std::string message = "Could not open: " + def.string() + ", FILE* empty";

			throw std::runtime_error(message);
		}

		const int caseSensitive{ 0 };

		const auto parseResult = LefDefParser::defrRead(defPtr, def.string().c_str(), nullptr, caseSensitive);

		if (parseResult != 0)
		{
			const std::string message = "Parsing: " + def.string() + " failed";

			throw std::runtime_error(message);
		}

		return m_data;
	}


	void Loader::initialize()
	{
		LefDefParser::defrInit();
		initializeCallbacks();
	}

	void Loader::initializeCallbacks()
	{
		LefDefParser::defrSetDieAreaCbk(onDieArea);
		LefDefParser::defrSetTrackCbk(onTracks);
		LefDefParser::defrSetGcellGridCbk(onGCellGrid);
		LefDefParser::defrSetViaStartCbk(onStartVia);
		LefDefParser::defrSetViaCbk(onVia);
		LefDefParser::defrSetSNetStartCbk(onSpecialNetsNumber);
		LefDefParser::defrSetComponentStartCbk(onComponentsNumber);
		LefDefParser::defrSetComponentCbk(onComponent);
		LefDefParser::defrSetNetCbk(onNet);
		LefDefParser::defrSetPinCbk(onPin);
	}

	bool Loader::verify(const std::filesystem::path& lef) const
	{
		return std::filesystem::is_regular_file(lef);
	}

	int Loader::onDieArea([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiBox* box, [[maybe_unused]]void* data)
	{
		m_data.dieArea = { {box->xl(), box->yl()},{box->xh(), box->yh()} };

		return 0;
	}

	int Loader::onTracks([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiTrack* track,[[maybe_unused]]void* data)
	{
		Tracks inTracks;
		
		inTracks.macro = std::string{ track->macro() } == "X" ? Tracks::Macro::X : Tracks::Macro::Y;
		inTracks.start = track->x();
		inTracks.number = track->xNum();
		inTracks.step = track->xStep();

		for (int i = 0; i < track->numLayers(); ++i)
		{
			inTracks.layer = track->layer(i);
			break;
		}

		m_data.tracks.push_back(inTracks);

		return 0;
	}

	int Loader::onGCellGrid([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiGcellGrid* gcellgrid,[[maybe_unused]] void* data)
	{
		GCellGrid inGrid;

		inGrid.macro = std::string{ gcellgrid->macro() } == "X" ? GCellGrid::Macro::X : GCellGrid::Macro::Y;
		inGrid.start = gcellgrid->x();
		inGrid.number = gcellgrid->xNum();
		inGrid.step = gcellgrid->xStep();

		m_data.grids.push_back(inGrid);

		return 0;
	}

	int Loader::onStartVia([[maybe_unused]]LefDefParser::defrCallbackType_e type, int viaCount,[[maybe_unused]] void* data)
	{
		m_data.viaCount = viaCount;

		m_data.vias.reserve(viaCount);

		return 0;
	}

	int Loader::onVia([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiVia* via,[[maybe_unused]] void* data)
	{
		Via inVia;

		inVia.name = via->name();
		
		if (via->hasViaRule())
		{
			char* vrn{nullptr}, * bl{ nullptr }, * cll{ nullptr }, * tl{ nullptr };
			int xs{0}, ys{ 0 }, xcs{ 0 }, ycs{ 0 }, xbe{ 0 }, ybe{ 0 }, xte{ 0 }, yte{ 0 };
			int cr{ 0 }, cc{ 0 }, xo{ 0 }, yo{ 0 }, xbo{ 0 }, ybo{ 0 }, xto{ 0 }, yto{ 0 };
			via->viaRule(&vrn, &xs, &ys, &bl, &cll, &tl, &xcs,
				&ycs, &xbe, &ybe, &xte, &yte);
			inVia.viaRule = std::string{ vrn };
			inVia.cutSizeX = xs;
			inVia.cutSizeY = ys;
			inVia.bottomLayer = std::string{ bl };
			inVia.cutLayer = std::string{ cll };
			inVia.topLayer = std::string{ tl };
			inVia.cutSpacingX = xcs;
			inVia.cutSpacingY = ycs;
			inVia.botEnclosureX = xbe;
			inVia.botEnclosureY = ybe;
			inVia.topEnclosureX = xte;
			inVia.topEnclosureY = yte;
			
			if (via->hasRowCol())
			{
				via->rowCol(&cr, &cc);
				inVia.numCutRows = cr;
				inVia.numCutCols = cc;
			}

			if (via->hasOrigin())
			{
				(void)via->origin(&xo, &yo);
				inVia.originX = xo;
				inVia.originY = yo;
			}

			if (via->hasOffset()) 
			{
				(void)via->offset(&xbo, &ybo, &xto, &yto);
				inVia.botOffX = xbo;
				inVia.botOffY = ybo;
				inVia.topOffX = xto;
				inVia.topOffY = yto;
			}

			if (via->hasCutPattern())
			{
				inVia.pattern = via->cutPattern();
			}
		}
		else
		{
			for (int i = 0; i < via->numLayers(); i++) 
			{
				char* name{ nullptr };
				int xl{ 0 }, yl{ 0 }, xh{ 0 }, yh{ 0 };
				via->layer(i, &name, &xl, &yl, &xh, &yh);
				std::string layer{ name };
				inVia.viaGeometry[layer].push_back({ {xl,yl},{xh,yh} });
			}
		}

		m_data.vias.push_back(inVia);
		
		return 0;
	}

	int Loader::onSpecialNetsNumber([[maybe_unused]]LefDefParser::defrCallbackType_e type, int specialNetsCount, [[maybe_unused]]void* data)
	{
		m_data.specialNetsCount = specialNetsCount;

		return 0;
	}

	int Loader::onComponentsNumber([[maybe_unused]]LefDefParser::defrCallbackType_e type, int componentsCount,[[maybe_unused]] void* data)
	{
		m_data.componentsCount = componentsCount;

		m_data.components.reserve(componentsCount);

		return 0;
	}

	int Loader::onComponent([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiComponent* component,[[maybe_unused]] void* data)
	{
		Component inComponent;

		inComponent.name = component->name();
		inComponent.id = component->id();
		inComponent.placementX = component->placementX();
		inComponent.placementY = component->placementY();
		
		const std::string orientationStr = component->placementOrientStr();

		if (orientationStr == "N")
		{
			inComponent.orientation = Orientation::N;
		}
		else if (orientationStr == "E")
		{
			inComponent.orientation = Orientation::E;
		}
		else if (orientationStr == "S")
		{
			inComponent.orientation = Orientation::S;
		}
		else if (orientationStr == "W")
		{
			inComponent.orientation = Orientation::W;
		}
		else if (orientationStr == "FN")
		{
			inComponent.orientation = Orientation::FN;
		}
		else if (orientationStr == "FE")
		{
			inComponent.orientation = Orientation::FE;
		}
		else if (orientationStr == "FS")
		{
			inComponent.orientation = Orientation::FS;
		}
		else if (orientationStr == "FW")
		{
			inComponent.orientation = Orientation::FW;
		}

		m_data.components.push_back(inComponent);

		return 0;
	}

	int Loader::onNet([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiNet* net, [[maybe_unused]]void* data)
	{
		Net inNet;

		inNet.name = net->name();

		for (int i = 0; i < net->numConnections(); ++i)
		{
			inNet.compPinPairs.push_back({ net->instance(i),net->pin(i) });
		}

		m_data.nets.push_back(inNet);

		return 0;
	}

	int Loader::onPin([[maybe_unused]]LefDefParser::defrCallbackType_e type, LefDefParser::defiPin* pin, [[maybe_unused]]void* data)
	{
		Pin inPin;

		inPin.name = pin->pinName();
		inPin.net = pin->netName();

		int xl{ 0 }, yl{ 0 }, xh{ 0 }, yh{ 0 };

		for (int i = 0; i < pin->numLayer(); ++i)
		{
			inPin.layer = pin->layer(i);
			
			pin->bounds(i, &xl, &yl, &xh, &yh);
		}

		inPin.bounds = { { xl,yl }, { xh,yh } };

		if (pin->hasPlacement())
		{
			inPin.placementX = pin->placementX();
			inPin.placementY = pin->placementY();
		}

		const std::string orientationStr = pin->orientStr();

		if (orientationStr == "N")
		{
			inPin.orientation = Orientation::N;
		}
		else if (orientationStr == "E")
		{
			inPin.orientation = Orientation::E;
		}
		else if (orientationStr == "S")
		{
			inPin.orientation = Orientation::S;
		}
		else if (orientationStr == "W")
		{
			inPin.orientation = Orientation::W;
		}
		else if (orientationStr == "FN")
		{
			inPin.orientation = Orientation::FN;
		}
		else if (orientationStr == "FE")
		{
			inPin.orientation = Orientation::FE;
		}
		else if (orientationStr == "FS")
		{
			inPin.orientation = Orientation::FS;
		}
		else if (orientationStr == "FW")
		{
			inPin.orientation = Orientation::FW;
		}

		m_data.pins.push_back(std::move(inPin));

		return 0;
	}
}