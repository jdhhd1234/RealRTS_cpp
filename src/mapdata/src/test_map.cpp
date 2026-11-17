#include "mapdata/test_map.hpp"

#include <cmath>

namespace mapdata
{

namespace
{

Pos CellCenter(std::uint32_t x, std::uint32_t y)
{
    return Pos{(static_cast<float>(x) + 0.5f) * 1000.0f, (static_cast<float>(y) + 0.5f) * 1000.0f};
}

void Connect(Map& map, EdgeKind kind, std::uint32_t a, std::uint32_t b, float capacity)
{
    const Pos pa = map.nodes[a].pos;
    const Pos pb = map.nodes[b].pos;
    map.edges.push_back(Edge{kind, NodeId{a}, NodeId{b}, capacity, std::hypot(pb.x - pa.x, pb.y - pa.y), 0});
}

}

Baked MakeTestMap()
{
    Baked baked;
    Map& map = baked.map;
    CellGrid& grid = map.grid;

    grid.width = 24;
    grid.height = 16;
    const std::size_t cells = std::size_t{grid.width} * grid.height;
    grid.terrain.assign(cells, Terrain::Plains);
    grid.region.assign(cells, RegionId{0});

    for (std::uint32_t y = 0; y < grid.height; ++y)
    {
        for (std::uint32_t x = 0; x < grid.width; ++x)
        {
            const std::size_t cell = CellIndex(grid, x, y);
            if (x >= 2 && x <= 5 && ((y >= 3 && y <= 6) || (y >= 10 && y <= 12)))
            {
                grid.terrain[cell] = Terrain::Forest;
            }
            if (x >= 18 && x <= 21 && y >= 11 && y <= 13)
            {
                grid.terrain[cell] = Terrain::Mountain;
            }
            if (x == 12 && y != 8)
            {
                grid.terrain[cell] = Terrain::Water;
            }
            if (x >= 12)
            {
                grid.region[cell] = RegionId{1};
            }
        }
    }

    map.regions = {
        Region{"West", Production{100.0f, 100.0f, 200.0f, 10.0f}},
        Region{"East", Production{100.0f, 100.0f, 200.0f, 10.0f}},
    };

    map.nodes = {
        Node{"West Base", NodeKind::Station, CellCenter(2, 8)},
        Node{"West Junction", NodeKind::Junction, CellCenter(8, 8)},
        Node{"East Junction", NodeKind::Junction, CellCenter(15, 8)},
        Node{"East Base", NodeKind::Station, CellCenter(21, 8)},
    };

    // 5.4 기본 용량: 철도 3000, 도로 600 t/일
    Connect(map, EdgeKind::Rail, 0, 1, 3000.0f);
    Connect(map, EdgeKind::Rail, 1, 2, 3000.0f);
    Connect(map, EdgeKind::Rail, 2, 3, 3000.0f);
    Connect(map, EdgeKind::Road, 1, 2, 600.0f);

    ScenarioData& scenario = baked.scenario;
    scenario.legal = {0, 1};
    scenario.control = {0, 1};
    scenario.units = {
        UnitSpawn{"1st Blue Division", 0, CellCenter(10, 6)},
        UnitSpawn{"1st Red Division", 1, CellCenter(14, 10)},
    };
    scenario.bases = {Facility{NodeId{0}, 0}, Facility{NodeId{3}, 1}};
    scenario.depots = {Facility{NodeId{1}, 0}, Facility{NodeId{2}, 1}};

    return baked;
}

}
