#include "mapdata/map.hpp"

#include <format>

namespace mapdata
{

namespace
{

[[noreturn]] void Fail(const std::string& message)
{
    throw LoadError("map: " + message);
}

void ValidateGrid(const Map& map)
{
    const CellGrid& grid = map.grid;
    if (grid.width == 0 || grid.height == 0)
    {
        Fail("grid is empty");
    }
    if (grid.width > 2000 || grid.height > 2000) // 5.4: 최대 2000 x 2000 km
    {
        Fail(std::format("grid {}x{} is larger than 2000x2000", grid.width, grid.height));
    }
    if (!(grid.cellSize > 0.0f))
    {
        Fail("cellSize must be positive");
    }

    const std::size_t cells = std::size_t{grid.width} * grid.height;
    if (grid.terrain.size() != cells)
    {
        Fail(std::format("terrain has {} cells, grid needs {}", grid.terrain.size(), cells));
    }
    if (grid.region.size() != cells)
    {
        Fail(std::format("region grid has {} cells, grid needs {}", grid.region.size(), cells));
    }
    for (const Terrain terrain : grid.terrain)
    {
        if (terrain > Terrain::Water)
        {
            Fail(std::format("unknown terrain kind {}", static_cast<int>(terrain)));
        }
    }

    if (map.regions.empty())
    {
        Fail("no regions");
    }
    for (const RegionId region : grid.region)
    {
        if (ToIndex(region) >= map.regions.size())
        {
            Fail(std::format("cell region {} out of range ({} regions)", ToIndex(region), map.regions.size()));
        }
    }
}

bool Inside(const CellGrid& grid, Pos pos)
{
    // NaN은 모든 비교가 false라서 여기서 걸러진다.
    return pos.x >= 0.0f && pos.x <= grid.width * grid.cellSize &&
           pos.y >= 0.0f && pos.y <= grid.height * grid.cellSize;
}

void ValidateNetwork(const Map& map)
{
    for (const Node& node : map.nodes)
    {
        if (node.kind > NodeKind::Port)
        {
            Fail(std::format("node '{}' has unknown kind", node.name));
        }
        if (!Inside(map.grid, node.pos))
        {
            Fail(std::format("node '{}' is outside the map", node.name));
        }
    }

    for (std::size_t i = 0; i < map.edges.size(); ++i)
    {
        const Edge& edge = map.edges[i];
        if (edge.kind > EdgeKind::River)
        {
            Fail(std::format("edge {} has unknown kind", i));
        }
        if (ToIndex(edge.from) >= map.nodes.size() || ToIndex(edge.to) >= map.nodes.size())
        {
            Fail(std::format("edge {} points to missing node ({} nodes)", i, map.nodes.size()));
        }
        if (edge.from == edge.to)
        {
            Fail(std::format("edge {} connects a node to itself", i));
        }
        if (!(edge.capacity >= 0.0f))
        {
            Fail(std::format("edge {} has negative capacity", i));
        }
        if (!(edge.length > 0.0f))
        {
            Fail(std::format("edge {} has no length", i));
        }
    }
}

void ValidateFacilities(const Map& map, const std::vector<Facility>& facilities, const char* what)
{
    for (const Facility& facility : facilities)
    {
        if (ToIndex(facility.node) >= map.nodes.size())
        {
            Fail(std::format("{} points to missing node {}", what, ToIndex(facility.node)));
        }
        if (facility.side >= kSideCount)
        {
            Fail(std::format("{} has unknown side {}", what, facility.side));
        }
    }
}

void ValidateScenario(const Map& map, const ScenarioData& scenario)
{
    if (scenario.legal.size() != map.regions.size() || scenario.control.size() != map.regions.size())
    {
        Fail(std::format("scenario region ownership has {}/{} entries, map has {} regions",
                         scenario.legal.size(), scenario.control.size(), map.regions.size()));
    }
    for (std::size_t i = 0; i < map.regions.size(); ++i)
    {
        if (scenario.legal[i] >= kSideCount || scenario.control[i] >= kSideCount)
        {
            Fail(std::format("region '{}' has unknown side", map.regions[i].name));
        }
    }

    for (const UnitSpawn& unit : scenario.units)
    {
        if (unit.side >= kSideCount)
        {
            Fail(std::format("unit '{}' has unknown side {}", unit.name, unit.side));
        }
        if (!Inside(map.grid, unit.pos))
        {
            Fail(std::format("unit '{}' is outside the map", unit.name));
        }
    }

    ValidateFacilities(map, scenario.depots, "depot");
    ValidateFacilities(map, scenario.bases, "base");
}

}

void Validate(const Baked& baked)
{
    ValidateGrid(baked.map);
    ValidateNetwork(baked.map);
    ValidateScenario(baked.map, baked.scenario);
}

}
