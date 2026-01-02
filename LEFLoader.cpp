#include "LEFLoader.hpp"

#include "lefdefParser/lefrReader.hpp"

#include <cstdio>

namespace in::lef
{

Loader::~Loader()
{
	LefDefParser::lefrClear();
	m_data = Data{};
}

Data Loader::get(const std::filesystem::path& lef)
{
	initialize();

	if (!verify(lef))
	{
		const std::string message = "Could not open: " + lef.string();

		throw std::runtime_error(message);
	}

	std::FILE* lefPtr = std::fopen(lef.string().c_str(), "r");

	if (lefPtr == nullptr)
	{
		const std::string message = "Could not open: " + lef.string() + ", FILE* empty";

		throw std::runtime_error(message);
	}

	const auto parseResult = LefDefParser::lefrRead(lefPtr, lef.string().c_str(), nullptr);

	if (parseResult != 0)
	{
		const std::string message = "Parsing: " + lef.string() + " failed";

		throw std::runtime_error(message);
	}

	return m_data;
}


void Loader::initialize()
{
	LefDefParser::lefrInit();
	initializeCallbacks();
}

void Loader::initializeCallbacks()
{
	LefDefParser::lefrSetSiteCbk(onSite);
	LefDefParser::lefrSetVersionCbk(onVersion);
	LefDefParser::lefrSetBusBitCharsCbk(onBusbit);
	LefDefParser::lefrSetDividerCharCbk(onDividerChar);
	LefDefParser::lefrSetUnitsCbk(onUnits);
	LefDefParser::lefrSetManufacturingCbk(onManufacturingGrid);
	LefDefParser::lefrSetClearanceMeasureCbk(onClearanceMeasure);
	LefDefParser::lefrSetUseMinSpacingCbk(onUseMinSpacing);
	LefDefParser::lefrSetLayerCbk(onLayer);
	LefDefParser::lefrSetViaCbk(onVia);
	LefDefParser::lefrSetMacroCbk(onMacro);
	LefDefParser::lefrSetPinCbk(onPin);
	LefDefParser::lefrSetObstructionCbk(onObstruction);
}

bool Loader::verify(const std::filesystem::path& lef) const
{
	return std::filesystem::is_regular_file(lef);
}

int Loader::onSite(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiSite* site, void* data)
{
	Site inputSite;
	inputSite.name = std::string{ site->name() };
	if (site->hasSize())
	{
		inputSite.sizeX = site->sizeX();
		inputSite.sizeY = site->sizeY();
	}
	if (site->hasClass())
	{
		inputSite.siteClass = std::string{ site->siteClass() } == "CORE" ? in::lef::Site::Class::CORE : in::lef::Site::Class::PAD;
	}
	m_data.sites.push_back(inputSite);

	return 0;
}

int Loader::onVersion(LefDefParser::lefrCallbackType_e cbType, double version, void* data)
{
	m_data.version = version;

	return 0;
}

int Loader::onBusbit(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data)
{
	m_data.busbitChars = str;

	return 0;
}

int Loader::onDividerChar(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data)
{
	m_data.dividerChar = str;

	return 0;
}

int Loader::onUnits(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiUnits* units, void* data)
{
	if (units->hasDatabase())
	{
		m_data.dbUnits = units->databaseNumber();
	}

	return 0;
}

int Loader::onManufacturingGrid(LefDefParser::lefrCallbackType_e cbType, double grid, void* data)
{
	m_data.manufacturingGrid = grid;

	return 0;
}

int Loader::onClearanceMeasure(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data)
{
	m_data.clearanceMeasure = std::string{ str } == "EUCLIDEAN" ? in::lef::Data::ClearanceMeasure::EUCLIDEAN : in::lef::Data::ClearanceMeasure::MAXXY;

	return 0;
}

int Loader::onUseMinSpacing(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiUseMinSpacing* spacing, void* data)
{
	if (std::string{ spacing->name() } == "OBS")
	{
		m_data.useMinSpacing = spacing->value() > 0 ? in::lef::Data::UseMinSpacing::ON : in::lef::Data::UseMinSpacing::OFF;
	}

	return 0;
}

int Loader::onLayer(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiLayer* layer, void* data)
{
	Layer inLayer;

	inLayer.name = layer->name();

	if (layer->hasType())
	{
		inLayer.type = std::string{ layer->type() } == "CUT" ? in::lef::Layer::Type::CUT : in::lef::Layer::Type::ROUTING;
	}

	if (layer->hasDirection())
	{
		inLayer.direction = std::string{ layer->direction() } == "HORIZONTAL" ? in::lef::Layer::Direction::HORIZONTAL : in::lef::Layer::Direction::VERTICAL;
	}

	if (layer->hasXYPitch())
	{
		inLayer.pitch = { layer->pitchX(), layer->pitchY() };
	}

	if (layer->hasWidth())
	{
		inLayer.width = layer->width();
	}

	if (layer->hasMinwidth())
	{
		inLayer.minwidth = layer->minwidth();
	}

	if (layer->hasArea())
	{
		inLayer.area = layer->area();
	}

	if (layer->hasSpacingNumber())
	{
		for (int i = 0; i < layer->numSpacing(); ++i)
		{
			Spacing inSpacing;
			inSpacing.value = layer->spacing(i);

			if (layer->hasSpacingAdjacent(i)) {
				inSpacing.adjCuts = layer->spacingAdjacentCuts(i);
				inSpacing.adjWithin = layer->spacingAdjacentWithin(i);
			}

			if (layer->hasSpacingEndOfLine(i))
			{
				inSpacing.eolWidth = layer->spacingEolWidth(i);
				inSpacing.eolWithin = layer->spacingEolWithin(i);

				if (layer->hasSpacingParellelEdge(i))
				{
					inSpacing.parSpace = layer->spacingParSpace(i);
					inSpacing.parWithin = layer->spacingParWithin(i);
				}
			}

			inLayer.spacings.push_back(inSpacing);
		}

		for (int i = 0; i < layer->numSpacingTable(); ++i)
		{
			SpacingEntry inSpacingEntry;
			const auto* spTable = layer->spacingTable(i);
			if (spTable->isParallel())
			{
				const auto* parallel = spTable->parallel();

				for (int j = 0; j < parallel->numLength(); ++j)
				{
					inSpacingEntry.prlLengths.push_back(parallel->length(i));
				}

				for (int j = 0; j < parallel->numWidth(); ++j)
				{
					inSpacingEntry.prlWidths.push_back(parallel->width(i));
				}

				for (int j = 0; j < parallel->numWidth(); ++j)
				{
					std::vector<double> spaces;

					for (int k = 0; k < parallel->numLength(); ++k)
					{
						spaces.push_back(parallel->widthSpacing(j,k));
					}
					inSpacingEntry.prlWidthSpacings.push_back(spaces);
				}
			}
			inLayer.spacingTable.push_back(inSpacingEntry);
		}
	}

	for(int i = 0; i< layer->numProps(); ++i)
	{
		if (!layer->propIsString(i))
		{
			continue;
		}
		
		inLayer.property = layer->propValue(i);
		inLayer.prCornerSpacing = parsePropertyCornerSpacing(inLayer.property);
		break;
	}
	
	m_data.layers.push_back(inLayer);

	return 0;
}

int Loader::onVia(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiVia* via, void* data)
{
	Via inVia;
	inVia.name = via->name();
	for (int i = 0; i < via->numLayers(); ++i)
	{
		const std::string layerName = via->layerName(i);
		Via::BoxList boxList;

		for (int j = 0; j < via->numRects(i); ++j)
		{
			boxList.push_back({ { via->lefiVia::xl(i, j), via->lefiVia::yl(i, j) },
				{via->lefiVia::xh(i, j), via->lefiVia::yh(i, j) } });
		}

		inVia.geometry[layerName] = boxList;
	}

	m_data.vias.push_back(inVia);

	return 0;
}

int Loader::onMacro(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiMacro* macro, void* data)
{
	Macro inMacro;
	inMacro.name = macro->name();
	
	if (macro->hasClass())
	{
		inMacro.macroClass = std::string{ macro->name() } == "CORE" ? in::lef::Site::Class::CORE : in::lef::Site::Class::PAD;
	}

	if (macro->hasOrigin())
	{
		inMacro.originX = macro->originX();
		inMacro.originY = macro->originY();
	}

	if (macro->hasSize())
	{
		inMacro.sizeX = macro->sizeX();
		inMacro.sizeY = macro->sizeY();
	}

	if (macro->hasSiteName())
	{
		inMacro.siteName = macro->siteName();
	}

	

	inMacro.hasXSymmetry = macro->hasXSymmetry() != 0;
	inMacro.hasYSymmetry = macro->hasYSymmetry() != 0;
	inMacro.has90Symmetry = macro->has90Symmetry() != 0;

	inMacro.pins = m_data.pins;
	inMacro.obstruction = m_data.obstruction;

	m_data.obstruction = Obs{};
	m_data.pins.clear();
	m_data.macros.push_back(inMacro);

	return 0;
}

int Loader::onPin(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiPin* pin, void* data)
{
	Pin inPin;
	inPin.name = pin->name();
	
	if (pin->hasDirection())
	{
		const std::string direction = pin->direction();

		if (direction == "INPUT")
		{
			inPin.direction = Pin::Direction::INPUT;
		}
		else if (direction == "OUTPUT")
		{
			inPin.direction = Pin::Direction::OUTPUT;
		}
		else if (direction == "INOUT")
		{
			inPin.direction = Pin::Direction::INOUT;
		}
	}

	if (pin->hasUse())
	{
		const std::string use = pin->use();

		if (use == "ANALOG")
		{
			inPin.use = Pin::Use::ANALOG;
		}
		else if (use == "CLOCK")
		{
			inPin.use = Pin::Use::CLOCK;
		}
		else if (use == "GROUND")
		{
			inPin.use = Pin::Use::GROUND;
		}
		else if (use == "POWER")
		{
			inPin.use = Pin::Use::POWER;
		}
		else if (use == "SIGNAL")
		{
			inPin.use = Pin::Use::SIGNAL;
		}
	}

	for (int i = 0; i < pin->numPorts(); ++i)
	{
		const auto* geometry = pin->port(i);
		std::string layerName;
		Pin::BoxList boxList;
		for (int j = 0; j < geometry->numItems(); ++j)
		{
			switch (geometry->itemType(j)) 
			{
			case lefiGeomLayerE:
				layerName = geometry->getLayer(j);
				break;
			case lefiGeomRectE:
				boxList.push_back({{ geometry->getRect(j)->xl, geometry->getRect(j)->yl},
					{geometry->getRect(j)->xh, geometry->getRect(j)->yh }});
				break;
			default:
				break;
			}
			inPin.pinGeometry[layerName] = boxList;
		}
	}

	m_data.pins.push_back(inPin);

	return 0;
}

int Loader::onObstruction(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiObstruction* obstr, void* data)
{
	const auto* obstrGeos = obstr->geometries();
	
	std::string layerName;
	Obs::BoxList boxList;
	for (int i = 0; i < obstrGeos->numItems(); ++i)
	{
		switch (obstrGeos->itemType(i))
		{
		case lefiGeomLayerE:
			boxList.clear();
			layerName = obstrGeos->getLayer(i);
			break;
		case lefiGeomRectE:
			boxList.push_back({ { obstrGeos->getRect(i)->xl, obstrGeos->getRect(i)->yl},
				{obstrGeos->getRect(i)->xh, obstrGeos->getRect(i)->yh } });
			break;
		default:
			break;
		}

		m_data.obstruction.obsGeometry[layerName] = boxList;
	}

	return 0;
}

}