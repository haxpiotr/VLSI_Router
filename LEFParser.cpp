#include "LEFParser.hpp"
#include "LEFDocument.hpp"

namespace lef
{
	std::optional<LEFDocument> Parser::parse(const std::string& input)
	{
        using namespace bp::literals;

        LEFDocument doc;

        auto setHeader = [&doc](auto& ctx)
            {
                auto rng = std::get<0>(_attr(ctx));
                doc.version.assign(rng.begin(), rng.end());
                doc.busbitchars = std::make_pair(std::get<0>(std::get<1>(_attr(ctx))), std::get<1>(std::get<1>(_attr(ctx))));
                doc.dividerChar = std::get<2>(_attr(ctx)); 
                doc.databaseUnit = std::get<3>(_attr(ctx));
                doc.manufacturingGrid = std::get<4>(_attr(ctx));

                const auto measure = std::get<5>(_attr(ctx));

                if (measure == "EUCLIDEAN")
                {
                    doc.clearanceMeasure = ClearanceMeasure::EUCLIDEAN;
                }
                else
                {
                    doc.clearanceMeasure = ClearanceMeasure::MAXXY;
                }

                doc.databaseUnit = std::get<6>(_attr(ctx));

                const auto minSpacing = std::get<7>(_attr(ctx));

                if (minSpacing == "ON")
                {
                    doc.useMinSpacingObs = UseMinSpacingObs::ON;
                }
                else
                {
                    doc.useMinSpacingObs = UseMinSpacingObs::OFF;
                }
            };

        auto setSite = [&doc](auto& ctx) 
            {
                Site site;
                site.name = std::get<0>(_attr(ctx));
                site.siteClass = std::get<1>(_attr(ctx)) == "CORE" ? Site::Class::CORE : Site::Class::PAD;
                site.width = std::get<2>(_attr(ctx));
                site.height = std::get<3>(_attr(ctx));
                
                if (site.name == std::get<4>(_attr(ctx)))
                {
                    doc.sites.push_back(site);
                }
                else
                {
                    _pass(ctx) = false;
                }
            };

        auto setLayer = [&doc](auto& ctx)
            {
                //auto const& [name,
                //    type_str,
                //    dir_opt,
                //    pitch_opt,
                //    width_opt,
                //    minwidth_opt,
                //    area_opt,
                //    spacingtable_opt,
                //    spacing_opt,
                //    end_name] = _attr(ctx);
            };

        auto const endLibraryStatement = bp::omit[endString >> bp::no_case[bp::string("LIBRARY")]];

        auto const grammar = makeHeaderParser()[setHeader]
             >> endLibraryStatement;

        if (const auto result = bp::parse(input, grammar, skipper); result)
        {
            return doc;
        }
	}

}