#pragma once

#include "DEFData.hpp"

#include "lefdefParser/defrReader.hpp"

#include <filesystem>

namespace in::def
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
		static int onDieArea(LefDefParser::defrCallbackType_e type, LefDefParser::defiBox* box, void* data);
		static int onTracks(LefDefParser::defrCallbackType_e type, LefDefParser::defiTrack* track, void* data);
		static int onGCellGrid(LefDefParser::defrCallbackType_e type, LefDefParser::defiGcellGrid* gcellgrid, void* data);
		static int onStartVia(LefDefParser::defrCallbackType_e type, int viaCount, void* data);
		static int onVia(LefDefParser::defrCallbackType_e type, LefDefParser::defiVia* via, void* data);
		static int onSpecialNetsNumber(LefDefParser::defrCallbackType_e type, int specialnetsCount, void* data);
		static int onComponentsNumber(LefDefParser::defrCallbackType_e type, int componentsNumber, void* data);
		static int onComponent(LefDefParser::defrCallbackType_e type, LefDefParser::defiComponent* component, void* data);
		static int onNet(LefDefParser::defrCallbackType_e type, LefDefParser::defiNet* net, void* data);
		static int onPin(LefDefParser::defrCallbackType_e type, LefDefParser::defiPin* pin, void* data);

	};
}