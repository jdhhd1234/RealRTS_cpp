#pragma once

#include <optional>
#include <vector>

#include "mapdata/map.hpp"

namespace sim
{

// 격자 A* (8방향, 4.5). 반환값은 지나갈 셀 중심들이고 마지막은 정확히 goal이다.
// 목표가 지도 밖·물이거나 닿을 수 없으면 nullopt.
// 같은 비용 경로는 셀 인덱스로 순서를 정해 항상 같은 경로가 나온다 (2.2 결정론).
std::optional<std::vector<mapdata::Pos>> FindPath(const mapdata::CellGrid& grid, mapdata::Pos from, mapdata::Pos goal);

}
