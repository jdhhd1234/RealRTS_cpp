#pragma once

#include <limits>

#include "mapdata/map.hpp"

namespace sim
{

// 셀 이동 비용 배율 [가정]. 평지 = 1. 물은 지나갈 수 없다 (도하는 나중).
constexpr float TerrainCost(mapdata::Terrain terrain)
{
    switch (terrain)
    {
    case mapdata::Terrain::Plains: return 1.0f;
    case mapdata::Terrain::Forest: return 2.0f;
    case mapdata::Terrain::Marsh: return 3.0f;
    case mapdata::Terrain::Urban: return 1.5f;
    case mapdata::Terrain::Mountain: return 4.0f;
    case mapdata::Terrain::Water: return std::numeric_limits<float>::infinity();
    }
    return std::numeric_limits<float>::infinity();
}

constexpr bool IsPassable(mapdata::Terrain terrain)
{
    return terrain != mapdata::Terrain::Water;
}

// 평지에서 1 tick(게임 1분)에 가는 거리 [가정]: 300 m = 18 km/h.
inline constexpr float kMarchMetersPerTick = 300.0f;

}
