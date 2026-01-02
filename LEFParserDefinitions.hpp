#pragma once

#include <boost/parser/parser.hpp>

namespace lef
{

	namespace bp = boost::parser;

	static constexpr auto comment = bp::lit('#') >> *(bp::char_ - '\n') >> -bp::lit('\n');
	static constexpr auto skipper = bp::ws | comment;
	static constexpr auto versionString = bp::lit("VERSION");
	static constexpr auto versionVal = bp::raw[bp::lexeme[bp::int_ >> '.' >> bp::int_ >> -('.' >> bp::int_)]];
	static constexpr auto dividerString = bp::lit("DIVIDERCHAR");
	static constexpr auto quotedChar = bp::lexeme['"' >> bp::char_ >> '"'];
	static constexpr auto busbitcharsString = bp::no_case[bp::lit("BUSBITCHARS")];
	static constexpr auto quotedCharPair = bp::lit('"') >> bp::char_ >> bp::char_ >> bp::lit('"');
	static constexpr auto manufacturingGridString = bp::no_case[bp::lit("MANUFACTURINGGRID")];
	static constexpr auto manufacturingGridVal = bp::double_;
	static constexpr auto clearanceMeasureString = bp::no_case[bp::lit("CLEARANCEMEASURE")];
	static constexpr auto clearanceMeasureVal = bp::no_case[bp::string("MAXXY") | bp::string("EUCLIDEAN")];
	static constexpr auto unitsString = bp::no_case[bp::lit("UNITS")];
	static constexpr auto databaseString = bp::no_case[bp::lit("DATABASE")];
	static constexpr auto micronsString = bp::no_case[bp::lit("MICRONS")];
	static constexpr auto endString = bp::no_case[bp::lit("END")];
	static constexpr auto databaseUnitsVal = unitsString >> databaseString >> micronsString >> bp::uint_ >> ';' >> endString >> unitsString;
	static constexpr auto useMinSpacingString = bp::no_case[bp::lit("USEMINSPACING")];
	static constexpr auto obsString = bp::no_case[bp::lit("OBS")];
	static constexpr auto useMinSpacingObsVal = bp::no_case[bp::string("ON") | bp::string("OFF")];
	static constexpr auto siteString = bp::lit("SITE");
	static constexpr auto siteVal = +bp::char_;
	static constexpr auto alphaOrUscore =
		bp::char_('A', 'Z') | bp::char_('a', 'z') | bp::char_('_');
	static constexpr auto identBodyChar =
		bp::char_('A', 'Z') | bp::char_('a', 'z') | bp::char_('0', '9')
		| bp::char_('_') | bp::char_('.') | bp::char_('-');
	static constexpr auto ident =
		bp::lexeme[alphaOrUscore >> *identBodyChar];

	static constexpr auto siteNameStatement = bp::lit("SITE") >> ident >> bp::lit("CLASS");
	static constexpr auto siteClassStatement = (bp::string("CORE") | bp::string("PAD")) >> ';';
	static constexpr auto siteSizeStatement = (bp::lit("SIZE") >> bp::double_ >> bp::lit("BY") >> bp::double_ >> ';');
	static constexpr auto endCoreSiteStatement =
		bp::omit[bp::no_case[bp::lit("END")]] >> ident;

	static constexpr auto layerStatement = bp::lit("LAYER") >> ident >>
		bp::lit("TYPE") >> (bp::string("CUT") | bp::string("ROUTING")) >> ';'
		>> -(bp::lit("DIRECTION") >> (bp::string("HORIZONTAL") | bp::string("VERTICAL")) >> ';')
			>> -(bp::lit("PITCH") >> bp::double_ >> bp::double_ >> ';')
			>> -(bp::lit("WIDTH") >> bp::double_ >> ';')
			>> -(bp::lit("MINWIDTH") >> bp::double_ >> ';')
			>> -(bp::lit("AREA") >> bp::double_ >> ';')
			>> -(bp::lit("SPACINGTABLE") >> bp::lit("PARALLELRUNLENGTH") >> bp::double_ >> +(bp::lit("WIDTH") >> bp::double_ >> bp::double_) >> ';')
			>> -(bp::lit("SPACING") >> bp::double_
				>> -(bp::lit("ENDOFLINE") >> bp::double_)
				>> -(bp::lit("WITHIN") >> bp::double_)
				>> ';')
			
			>> bp::lit("END") >> ident;
			

}