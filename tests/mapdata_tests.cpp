#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>

#include "mapdata/json_io.hpp"
#include "mapdata/test_map.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("test map is valid and has expected shape")
{
    const mapdata::Baked baked = mapdata::MakeTestMap();
    REQUIRE_NOTHROW(mapdata::Validate(baked));

    const mapdata::CellGrid& grid = baked.map.grid;
    REQUIRE(grid.width == 24);
    REQUIRE(grid.height == 16);
    REQUIRE(grid.terrain.size() == 24 * 16);
    REQUIRE(grid.terrain[mapdata::CellIndex(grid, 12, 0)] == mapdata::Terrain::Water);
    REQUIRE(grid.terrain[mapdata::CellIndex(grid, 12, 8)] == mapdata::Terrain::Plains); // 다리
    REQUIRE(grid.region[mapdata::CellIndex(grid, 11, 0)] == mapdata::RegionId{0});
    REQUIRE(grid.region[mapdata::CellIndex(grid, 12, 0)] == mapdata::RegionId{1});

    REQUIRE(baked.map.nodes.size() == 4);
    REQUIRE(baked.map.edges.size() == 4);
    REQUIRE(baked.map.edges[0].length == 6000.0f); // 셀 (2,8) -> (8,8)
    REQUIRE(baked.scenario.units.size() == 2);
}

TEST_CASE("JSON round trip keeps everything")
{
    const mapdata::Baked original = mapdata::MakeTestMap();
    const std::string json = mapdata::ToJson(original);
    const mapdata::Baked loaded = mapdata::FromJson(json);

    REQUIRE(mapdata::ToJson(loaded) == json);
    REQUIRE(loaded.map.nodes[1].name == "West Junction");
    REQUIRE(loaded.scenario.depots[1].node == mapdata::NodeId{2});
}

TEST_CASE("file save and load round trip")
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "realrts_mapdata_test.json";
    const mapdata::Baked original = mapdata::MakeTestMap();

    mapdata::SaveJson(original, path);
    const mapdata::Baked loaded = mapdata::LoadJson(path);
    std::filesystem::remove(path);

    REQUIRE(mapdata::ToJson(loaded) == mapdata::ToJson(original));
}

TEST_CASE("loader rejects broken input with a reason")
{
    REQUIRE_THROWS_WITH(mapdata::FromJson("{"), ContainsSubstring("map JSON"));
    REQUIRE_THROWS_WITH(mapdata::FromJson("{}"), ContainsSubstring("map JSON"));
    REQUIRE_THROWS_WITH(mapdata::LoadJson("does/not/exist.json"), ContainsSubstring("cannot open"));

    // 호출하는 쪽(app)은 LoadError 하나만 잡으면 된다.
    REQUIRE_THROWS_AS(mapdata::FromJson("{"), mapdata::LoadError);
    REQUIRE_THROWS_AS(mapdata::LoadJson("does/not/exist.json"), mapdata::LoadError);
    mapdata::Baked baked = mapdata::MakeTestMap();
    baked.map.grid.width = 0;
    REQUIRE_THROWS_AS(mapdata::Validate(baked), mapdata::LoadError);
}

TEST_CASE("validation catches bad indices and sizes")
{
    SECTION("edge to missing node")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.map.edges[0].to = mapdata::NodeId{99};
        REQUIRE_THROWS_WITH(mapdata::FromJson(mapdata::ToJson(baked)), ContainsSubstring("missing node"));
    }
    SECTION("terrain size does not match grid")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.map.grid.terrain.pop_back();
        REQUIRE_THROWS_WITH(mapdata::Validate(baked), ContainsSubstring("terrain has"));
    }
    SECTION("cell points to missing region")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.map.grid.region[0] = mapdata::RegionId{5};
        REQUIRE_THROWS_WITH(mapdata::Validate(baked), ContainsSubstring("out of range"));
    }
    SECTION("unit outside the map")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.scenario.units[0].pos.x = -1.0f;
        REQUIRE_THROWS_WITH(mapdata::Validate(baked), ContainsSubstring("outside the map"));
    }
    SECTION("unknown side")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.scenario.bases[0].side = 7;
        REQUIRE_THROWS_WITH(mapdata::Validate(baked), ContainsSubstring("unknown side"));
    }
    SECTION("region ownership count mismatch")
    {
        mapdata::Baked baked = mapdata::MakeTestMap();
        baked.scenario.legal.pop_back();
        REQUIRE_THROWS_WITH(mapdata::Validate(baked), ContainsSubstring("ownership"));
    }
}
