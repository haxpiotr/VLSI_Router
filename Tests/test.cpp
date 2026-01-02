
#include "gtest/gtest.h"

#include "../LEFLoader.hpp"
#include "../DEFLoader.hpp"
#include "../DataTransformer.hpp"
#include "../GlobalRouter.hpp"

TEST(LefParser, ShouldParseSite) 
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd18_sample.input.lef");
	EXPECT_EQ(result.sites.size(), 1);
	EXPECT_EQ(result.sites.front().name, "CoreSite");
	EXPECT_DOUBLE_EQ(result.sites.front().sizeX, 0.2);
	EXPECT_DOUBLE_EQ(result.sites.front().sizeY, 1.71);
}

TEST(LefParser, ShouldParseHeader)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd18_sample.input.lef");
	EXPECT_EQ(result.sites.size(), 1);
	EXPECT_EQ(result.busbitChars, "[]");
	EXPECT_EQ(result.dividerChar, "/");
	EXPECT_EQ(result.clearanceMeasure, in::lef::Data::ClearanceMeasure::EUCLIDEAN);
	EXPECT_EQ(result.useMinSpacing, in::lef::Data::UseMinSpacing::ON);
	EXPECT_DOUBLE_EQ(result.version, 5.8);
	EXPECT_DOUBLE_EQ(result.dbUnits, 2000);
	EXPECT_DOUBLE_EQ(result.manufacturingGrid, 0.0005);
}

TEST(LefParser, ShouldParseLayers)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd18_sample.input.lef");
	EXPECT_EQ(result.sites.size(), 1);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.layers.front().name, "Metal1");
	EXPECT_EQ(result.layers.front().type, in::lef::Layer::Type::ROUTING);
	EXPECT_EQ(result.layers.front().direction, in::lef::Layer::Direction::HORIZONTAL);
	const std::pair<double, double> expectedPitch{ 0.19,0.19 };
	EXPECT_EQ(result.layers.front().pitch, expectedPitch);
}

TEST(LefParser, ShouldParseComplexLayers)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.layers.front().name, "Metal1");
	EXPECT_EQ(result.layers.front().type, in::lef::Layer::Type::ROUTING);
	EXPECT_EQ(result.layers.front().direction, in::lef::Layer::Direction::VERTICAL);
	const std::pair<double, double> expectedPitch{ 0.1,0.1 };
	EXPECT_EQ(result.layers.front().pitch, expectedPitch);
	EXPECT_EQ(result.layers.front().spacings.size(), 1);
	const std::string name{ "Metal4" };
	const auto metal4It = std::find_if(result.layers.begin(), result.layers.end(), [&name](const auto& layer)
		{
			return layer.name == name;
		});
	EXPECT_NE(metal4It, result.layers.end());
	const auto& metal4 = *metal4It;
	const std::pair<double, double> metal4Pitch{ 0.1,0.1 };
	EXPECT_EQ(metal4.pitch, metal4Pitch);
	EXPECT_EQ(metal4.spacings.size(), 2);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().adjCuts, 0);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().value, 0.08);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().eolWidth, 0.08);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().eolWithin, 0.025);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().parSpace, 0);
	EXPECT_DOUBLE_EQ(metal4.spacings.front().parWithin, 0);
	EXPECT_DOUBLE_EQ(metal4.spacings.back().parSpace, 0.1);
	EXPECT_DOUBLE_EQ(metal4.spacings.back().parWithin, 0.025);
	EXPECT_EQ(metal4.spacingTable.size(), 1);
	EXPECT_EQ(metal4.spacingTable.front().prlLengths.size(), 5);
	EXPECT_EQ(metal4.spacingTable.front().prlWidths.size(), 6);
	const std::vector<double> expectedSpacingRow{ 0.05, 0.10, 0.13,0.15, 0.15 };
	EXPECT_EQ(metal4.spacingTable.front().prlWidthSpacings[4], expectedSpacingRow);
	EXPECT_FALSE(metal4.property.empty());
	EXPECT_TRUE(metal4.prCornerSpacing.has_value());
	EXPECT_DOUBLE_EQ(metal4.prCornerSpacing.value().exceptEol, 0.08);
	EXPECT_EQ(metal4.prCornerSpacing.value().widthsAndSpacings.size(), 3);
	EXPECT_DOUBLE_EQ(metal4.prCornerSpacing.value().widthsAndSpacings[1].first, 0.2);
	EXPECT_DOUBLE_EQ(metal4.prCornerSpacing.value().widthsAndSpacings[1].second, 0.2);
}

TEST(LefParser, ShouldParseVias)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.vias.size(), 67);
}

TEST(LefParser, ShouldParseMacros)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.vias.size(), 67);
	EXPECT_EQ(result.macros.size(), 27);
	EXPECT_EQ(result.macros.front().name, "XNOR2X1");
	EXPECT_EQ(result.macros.back().name, "PDO04CDG");
	EXPECT_EQ(result.macros.front().siteName, "CoreSite");
	EXPECT_EQ(result.macros.back().siteName, "CoreSite");
	EXPECT_DOUBLE_EQ(result.macros.front().originX, 0);
	EXPECT_DOUBLE_EQ(result.macros.back().originX, 0);
	EXPECT_DOUBLE_EQ(result.macros.front().sizeX,1.4);
	EXPECT_DOUBLE_EQ(result.macros.front().sizeY, 1.2);
	EXPECT_DOUBLE_EQ(result.macros.back().sizeX, 4.0);
	EXPECT_DOUBLE_EQ(result.macros.back().sizeY, 23.5);
}

TEST(LefParser, ShouldParsePins)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.vias.size(), 67);
	EXPECT_EQ(result.macros.size(), 27);
	EXPECT_EQ(result.macros.front().pins.size(), 5);
	EXPECT_EQ(result.macros.front().pins.front().name, "A");
	EXPECT_EQ(result.macros.front().pins.front().pinGeometry.size(), 1);
	const auto& geo = result.macros.front().pins.front().pinGeometry;
	const auto actualX = geo.at("Metal1").front().min_corner().x();
	const auto actualY = geo.at("Metal1").front().min_corner().y();
	EXPECT_DOUBLE_EQ(actualX, 0.440000);
	EXPECT_DOUBLE_EQ(actualY, 0.557000);
}

TEST(LefParser, ShouldParsePinsWithManyLAyers)
{
	in::lef::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.vias.size(), 67);
	EXPECT_EQ(result.macros.size(), 27);
	const std::string actualName{ "PDIDGZ" };
	const auto multiLayerPinMacroIt = std::find_if(result.macros.begin(), result.macros.end(), [&actualName](const auto& macro) {
		return macro.name == actualName; });
	const auto& multiLayerPinMacro = *multiLayerPinMacroIt;
	EXPECT_EQ(multiLayerPinMacro.pins.size(), 2);
	EXPECT_EQ(multiLayerPinMacro.pins.at(0).pinGeometry.size(), 5);
}

TEST(LefParser, ShouldParseObs)
{
	in::lef::Loader loader;
	const auto result = loader.get("iData/spd19_sample4.input.lef");
	EXPECT_EQ(result.sites.size(), 3);
	EXPECT_EQ(result.layers.size(), 18);
	EXPECT_EQ(result.vias.size(), 67);
	EXPECT_EQ(result.macros.size(), 27);
	EXPECT_EQ(result.macros.back().name, "PDO04CDG");
	EXPECT_EQ(result.macros.back().obstruction.obsGeometry.size(), 11);
	const auto& geo = result.macros.back().obstruction.obsGeometry;
	const auto actGeo = geo.at("Metal2");
	EXPECT_EQ(actGeo.size(), 5);

}

TEST(DefParser, ShouldParseDieArea)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.dieArea.min_corner().x(), 0);
	EXPECT_EQ(result.dieArea.min_corner().y(), 0);
	EXPECT_EQ(result.dieArea.max_corner().x(), 390600);
	EXPECT_EQ(result.dieArea.max_corner().y(), 390000);
}

TEST(DefParser, ShouldParseTracks)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.tracks.size(),18);
	EXPECT_EQ(result.tracks.front().layer, "Metal9");
	EXPECT_EQ(result.tracks.front().macro, in::def::Tracks::Macro::Y);
	EXPECT_DOUBLE_EQ(result.tracks.front().start, 200);
	EXPECT_DOUBLE_EQ(result.tracks.front().number, 975);
	EXPECT_DOUBLE_EQ(result.tracks.front().step, 400);
	EXPECT_EQ(result.tracks.back().layer, "Metal1");
	EXPECT_EQ(result.tracks.back().macro, in::def::Tracks::Macro::X);
	EXPECT_DOUBLE_EQ(result.tracks.back().start, 100);
	EXPECT_DOUBLE_EQ(result.tracks.back().number, 1953);
	EXPECT_DOUBLE_EQ(result.tracks.back().step, 200);
}

TEST(DefParser, ShouldParseGCellGrid)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.grids.size(), 6);
	EXPECT_EQ(result.grids.front().macro, in::def::GCellGrid::Macro::X);
	EXPECT_DOUBLE_EQ(result.grids.front().start, 390100);
	EXPECT_DOUBLE_EQ(result.grids.front().number, 2);
	EXPECT_DOUBLE_EQ(result.grids.front().step, 500);
	EXPECT_EQ(result.grids.back().macro, in::def::GCellGrid::Macro::Y);
	EXPECT_DOUBLE_EQ(result.grids.back().start, 0);
	EXPECT_DOUBLE_EQ(result.grids.back().number, 2);
	EXPECT_DOUBLE_EQ(result.grids.back().step, 200);
}

TEST(DefParser, ShouldParseVias)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.viaCount, 248);
	EXPECT_EQ(result.vias.size(), 248);
	EXPECT_EQ(result.vias.front().name, "via1Array_22");
	EXPECT_EQ(result.vias.front().viaGeometry.size(), 3);
	EXPECT_EQ(result.vias.front().viaGeometry.at("Metal2").size(), 1);
	EXPECT_EQ(result.vias.front().viaGeometry.at("Metal2").back().min_corner().x(), -800);
	EXPECT_EQ(result.vias.front().viaGeometry.at("Metal2").back().min_corner().y(), -800);
	EXPECT_EQ(result.vias.front().viaGeometry.at("Metal2").back().max_corner().x(), 800);
	EXPECT_EQ(result.vias.front().viaGeometry.at("Metal2").back().max_corner().y(), 800);
	EXPECT_EQ(result.vias[1].name, "via2Array_24");
	EXPECT_EQ(result.vias[1].viaGeometry.size(), 3);
	EXPECT_EQ(result.vias[1].viaGeometry.at("Metal3").size(), 1);
	EXPECT_EQ(result.vias[1].viaGeometry.at("Metal3").back().min_corner().x(), -800);
	EXPECT_EQ(result.vias[1].viaGeometry.at("Metal3").back().min_corner().y(), -800);
	EXPECT_EQ(result.vias[1].viaGeometry.at("Metal3").back().max_corner().x(), 800);
	EXPECT_EQ(result.vias[1].viaGeometry.at("Metal3").back().max_corner().y(), 800);
	EXPECT_EQ(result.vias[2].name, "via5Array_2");
	EXPECT_EQ(result.vias[2].viaGeometry.size(), 0);
	EXPECT_EQ(result.vias[2].viaRule, "M6_M5");
	EXPECT_EQ(result.vias[2].cutSizeX, 720);
	EXPECT_EQ(result.vias[2].cutSizeY, 720);
	EXPECT_EQ(result.vias[2].topLayer, "Metal6");
	EXPECT_EQ(result.vias[2].cutLayer, "Via5");
	EXPECT_EQ(result.vias[2].bottomLayer, "Metal5");
	EXPECT_EQ(result.vias[2].cutSpacingX, 700);
	EXPECT_EQ(result.vias[2].cutSpacingY, 700);
	EXPECT_EQ(result.vias[2].botEnclosureX, 410);
	EXPECT_EQ(result.vias[2].botEnclosureY, 410);
	EXPECT_EQ(result.vias[2].topEnclosureX, 410);
	EXPECT_EQ(result.vias[2].topEnclosureY, 410);
	EXPECT_EQ(result.vias[2].numCutRows, 14);
	EXPECT_EQ(result.vias[2].numCutCols, 14);
}

TEST(DefParser, ShouldParseSpecialNetsCount)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.specialNetsCount, 2);
}

TEST(DefParser, ShouldParseComponentsCount)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.componentsCount, 67);
}

TEST(DefParser, ShouldParseComponents)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd19_sample4.input.def");
	EXPECT_EQ(result.componentsCount, 67);
	EXPECT_EQ(result.components.size(), 67);
	EXPECT_EQ(result.components[2].id, "i_3");
	EXPECT_EQ(result.components[2].name, "INVX2");
	EXPECT_EQ(result.components[2].orientation, in::def::Orientation::FS);
	EXPECT_EQ(result.components[2].placementX, 302800);
	EXPECT_EQ(result.components[2].placementY, 201000);
}

TEST(DefParser, ShouldParseNets)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd18_test1.input.def");
	EXPECT_EQ(result.nets.size(), 3153);
}

TEST(DefParser, ShouldParsePins)
{
	in::def::Loader loader;
	const auto result = loader.get("Data/ispd18_test2.input.def");
	EXPECT_EQ(result.pins.size(), 1211);
}

TEST(DataTransformer, ShouldResizeAndMovePinsN)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test10.input.def");
	const auto library = lefLoader.get("Data/ispd18_test10.input.lef");
	in::DataTransformer transformer(library,design);
	
	const auto transPins = transformer.getPlacedPins();

	const size_t ioPinsCount = 1211;

	EXPECT_EQ(transPins.size(), 1391407 + ioPinsCount);
	std::pair<std::string, std::string> key = {"inst83110", "B"};
	auto it = std::find_if(transPins.begin(), transPins.end(), [&key](const auto& e)
		{
			return e.compIdPair == key;
		});
	const auto& keyPin = *it;
	EXPECT_EQ(keyPin.portGeometry.at("Metal1").size(), 5);
	const auto& keyGeo = keyPin.portGeometry.at("Metal1")[1];
	EXPECT_EQ(keyGeo.min_corner().x(), 302720);
	EXPECT_EQ(keyGeo.min_corner().y(), 1350780);
	EXPECT_EQ(keyGeo.max_corner().x(), 302880);
	EXPECT_EQ(keyGeo.max_corner().y(), 1351040);
}

TEST(DataTransformer, ShouldResizeRotateAndMovePinsFS)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test10.input.def");
	const auto library = lefLoader.get("Data/ispd18_test10.input.lef");
	in::DataTransformer transformer(library, design);

	const auto transPins = transformer.getPlacedPins();

	const size_t ioPinsCount = 1211;

	EXPECT_EQ(transPins.size(), 1391407 + ioPinsCount);
	std::pair<std::string, std::string> key = {"inst161819", "Q"};
	auto it = std::find_if(transPins.begin(), transPins.end(), [&key](const auto& e)
		{
			return e.compIdPair == key;
		});
	const auto& keyPin = *it;
	EXPECT_EQ(keyPin.portGeometry.at("Metal1").size(), 2);
	const auto& keyGeo = keyPin.portGeometry.at("Metal1")[1];
	EXPECT_EQ(keyGeo.min_corner().x(), 1387920);
	EXPECT_EQ(keyGeo.min_corner().y(), 584440);
	EXPECT_EQ(keyGeo.max_corner().x(), 1387800);
	EXPECT_EQ(keyGeo.max_corner().y(), 586000);
}

TEST(DataTransformer, ShouldCreateGlobalRoutingGrid)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test1.input.def");
	const auto library = lefLoader.get("Data/ispd18_test1.input.lef");
	in::DataTransformer transformer(library, design);

	const auto grid = transformer.getGlobalGrid(200,200);
	EXPECT_EQ(grid.getCount(), 40000);
	EXPECT_EQ(grid.getCellCapacity().first, 36);
	EXPECT_EQ(grid.getCellCapacity().second, 38);
}

TEST(DataTransformer, ShouldFindBBoxAndCenter)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test1.input.def");
	const auto library = lefLoader.get("Data/ispd18_test1.input.lef");
	in::DataTransformer transformer(library, design);

	const auto transPins = transformer.getPlacedPins();
	EXPECT_EQ(transPins.size(), 54059);
	EXPECT_EQ(transPins[1].portGeometry.at("Metal1").size(), 2);
	EXPECT_EQ(transPins[1].portGeometry.size(), 1);

	const auto& testPin = transPins[1];
	const auto& testBox1 = testPin.portGeometry.at("Metal1")[0];
	const auto& testBox2 = testPin.portGeometry.at("Metal1")[1];

	EXPECT_EQ(testBox1.min_corner().x(), 362120);
	EXPECT_EQ(testBox1.min_corner().y(), 381130);
	EXPECT_EQ(testBox1.max_corner().x(), 362280);
	EXPECT_EQ(testBox1.max_corner().y(), 381520);
	EXPECT_EQ(testBox2.min_corner().x(), 361100);
	EXPECT_EQ(testBox2.min_corner().y(), 381360);
	EXPECT_EQ(testBox2.max_corner().x(), 362280);
	EXPECT_EQ(testBox2.max_corner().y(), 381520);

	in::point_int expectedCenter{ 361690,381325 };
	in::box_int expectedBBox{{ 361100,381130 },{362280, 381520}};

	const auto testPinBBoxResult = in::getPinBBox(testPin);
	EXPECT_EQ(testPinBBoxResult.min_corner().x(), expectedBBox.min_corner().x());
	EXPECT_EQ(testPinBBoxResult.min_corner().y(), expectedBBox.min_corner().y());

	const auto testPinResult = in::getPinCenter(testPin);
	EXPECT_EQ(testPinResult.x(), expectedCenter.x());
	EXPECT_EQ(testPinResult.y(), expectedCenter.y());
}

TEST(DataTransformer, ShouldFindIndex)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test1.input.def");
	const auto library = lefLoader.get("Data/ispd18_test1.input.lef");
	in::DataTransformer transformer(library, design);
	const auto globalGrid = transformer.getGlobalGrid(100, 100);
	const auto index = globalGrid.getIndex({ 0,0 });
	EXPECT_EQ(index, 0);

	const auto count = globalGrid.getCount();
	EXPECT_EQ(count, 10000);

	const auto indexEnd = globalGrid.getIndex({ 390800 - 1, 383040 - 1});
	EXPECT_EQ(indexEnd, 9999);

	const auto indexMid = globalGrid.getIndex({ 390800/2 - 1, 383040/2 - 1 });
	EXPECT_EQ(indexMid, 4999);
}

TEST(DataTransformer, ShouldFindNeighbours)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test1.input.def");
	const auto library = lefLoader.get("Data/ispd18_test1.input.lef");
	in::DataTransformer transformer(library, design);
	const auto globalGrid = transformer.getGlobalGrid(100, 100);
	const auto index = globalGrid.getIndex({ 0,0 });
	EXPECT_EQ(index, 0);
	const auto indexNeighs = globalGrid.getNeighbours(index);
	EXPECT_EQ(indexNeighs.size(), 2);

	const auto itFirst = std::find(indexNeighs.begin(), indexNeighs.end(), 1);
	const auto itSecond = std::find(indexNeighs.begin(), indexNeighs.end(), 100);
	EXPECT_EQ(*itFirst, 1);
	EXPECT_EQ(*itSecond, 100);
}

TEST(GlobalRouter, ShouldGetNetlist)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test1.input.def");
	const auto library = lefLoader.get("Data/ispd18_test1.input.lef");
	in::DataTransformer transformer(library, design);
	const auto globalGrid = transformer.getGlobalGrid(100, 100);
	const auto index = globalGrid.getIndex({ 0,0 });
	EXPECT_EQ(index, 0);
	const auto indexNeighs = globalGrid.getNeighbours(index);
	EXPECT_EQ(indexNeighs.size(), 2);

	const auto itFirst = std::find(indexNeighs.begin(), indexNeighs.end(), 1);
	const auto itSecond = std::find(indexNeighs.begin(), indexNeighs.end(), 100);
	EXPECT_EQ(*itFirst, 1);
	EXPECT_EQ(*itSecond, 100);

	in::GlobalRouter GRouter(transformer, 100, 100);
	const auto& expectedNetlist = GRouter.getNetlist();
	EXPECT_EQ(expectedNetlist.size(), 3153);
}

TEST(GlobalRouter, ShouldGetBigNetlist)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test2.input.def");
	const auto library = lefLoader.get("Data/ispd18_test2.input.lef");
	in::DataTransformer transformer(library, design);
	const auto globalGrid = transformer.getGlobalGrid(100, 100);
	const auto index = globalGrid.getIndex({ 0,0 });
	EXPECT_EQ(index, 0);
	const auto indexNeighs = globalGrid.getNeighbours(index);
	EXPECT_EQ(indexNeighs.size(), 2);

	const auto itFirst = std::find(indexNeighs.begin(), indexNeighs.end(), 1);
	const auto itSecond = std::find(indexNeighs.begin(), indexNeighs.end(), 100);
	EXPECT_EQ(*itFirst, 1);
	EXPECT_EQ(*itSecond, 100);
	in::GlobalRouter GRouter(transformer, 100, 100);
	EXPECT_EQ(GRouter.getNetlist().size(), 36834);
}

TEST(GlobalRouter, ShouldGetEvenBiggerNetlist)
{
	in::def::Loader defLoader;
	in::lef::Loader lefLoader;
	const auto design = defLoader.get("Data/ispd18_test10.input.def");
	const auto library = lefLoader.get("Data/ispd18_test10.input.lef");
	in::DataTransformer transformer(library, design);
	const auto globalGrid = transformer.getGlobalGrid(100, 100);
	const auto index = globalGrid.getIndex({ 0,0 });
	EXPECT_EQ(index, 0);
	const auto indexNeighs = globalGrid.getNeighbours(index);
	EXPECT_EQ(indexNeighs.size(), 2);

	const auto itFirst = std::find(indexNeighs.begin(), indexNeighs.end(), 1);
	const auto itSecond = std::find(indexNeighs.begin(), indexNeighs.end(), 100);
	EXPECT_EQ(*itFirst, 1);
	EXPECT_EQ(*itSecond, 100);
	
	in::GlobalRouter GRouter(transformer, 100, 100);
	EXPECT_EQ(GRouter.getNetlist().size(), 182000);

	const auto histogram = GRouter.getNetlistElementHistogram();
		
	for (const auto& [key, count] : histogram)
	{
		std::cout << std::to_string(key) << ": " << std::to_string(count) << "\n";
	}

	std::cout << std::flush;

	EXPECT_EQ(histogram.size(), 79);
}
