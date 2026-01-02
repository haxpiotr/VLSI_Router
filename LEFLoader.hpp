#pragma once

#include "LEFData.hpp"

#include <filesystem>

namespace in::lef
{
	
	class Loader
	{
	public:
		Loader() = default;
		~Loader();
		[[nodiscard]] Data get(const std::filesystem::path& lef);

	private:
		void initialize();
		void initializeCallbacks();
		bool verify(const std::filesystem::path& lef) const;
		inline static Data m_data;
		static int onSite(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiSite* site, void* data);
		static int onVersion(LefDefParser::lefrCallbackType_e cbType, double version, void* data);
		static int onBusbit(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data);
		static int onDividerChar(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data);
		static int onUnits(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiUnits* units, void* data);
		static int onManufacturingGrid(LefDefParser::lefrCallbackType_e cbType, double grid, void* data);
		static int onClearanceMeasure(LefDefParser::lefrCallbackType_e cbType, const char* str, void* data);
		static int onUseMinSpacing(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiUseMinSpacing* spacing, void* data);
		static int onLayer(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiLayer* layer, void* data);
		static int onVia(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiVia* via, void* data);
		static int onMacro(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiMacro *macro, void* data);
		static int onPin(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiPin* pin, void* data);
		static int onObstruction(LefDefParser::lefrCallbackType_e cbType, LefDefParser::lefiObstruction* obstr, void* data);
	};

}