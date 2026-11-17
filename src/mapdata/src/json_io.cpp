#include "mapdata/json_io.hpp"

#include <fstream>
#include <iterator>

#include <nlohmann/json.hpp>

namespace mapdata
{

// enum과 강한 타입 ID는 nlohmann 기본 동작대로 숫자로 저장된다.
// 모든 키가 필수다: 빠진 키가 있으면 읽기 실패.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pos, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CellGrid, width, height, cellSize, terrain, region)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Production, fuel, ammo, food, equipment)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Region, name, production)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Node, name, kind, pos)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Edge, kind, from, to, capacity, length, gauge)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Map, grid, regions, nodes, edges)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UnitSpawn, name, side, pos)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Facility, node, side)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScenarioData, legal, control, units, depots, bases)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Baked, map, scenario)

std::string ToJson(const Baked& baked)
{
    return nlohmann::json(baked).dump();
}

Baked FromJson(std::string_view text)
{
    Baked baked;
    try
    {
        nlohmann::json::parse(text).get_to(baked);
    }
    catch (const nlohmann::json::exception& error)
    {
        throw LoadError(std::string("map JSON: ") + error.what());
    }
    Validate(baked);
    return baked;
}

void SaveJson(const Baked& baked, const std::filesystem::path& path)
{
    std::ofstream out(path, std::ios::binary);
    out << ToJson(baked);
    if (!out)
    {
        throw LoadError("cannot write " + path.string());
    }
}

Baked LoadJson(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        throw LoadError("cannot open " + path.string());
    }
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return FromJson(text);
}

}
