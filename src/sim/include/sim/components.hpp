#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mapdata/map.hpp"

namespace sim
{

// 부대 ID. 저장·리플레이·명령은 entt::entity가 아니라 이것을 쓴다 (4.1).
// 시나리오의 units 순서 그대로 0부터 매긴다.
enum class UnitId : std::uint32_t {};

struct UnitInfo
{
    UnitId id{};
    mapdata::Side side = 0;
    std::string name;
};

struct Position
{
    mapdata::Pos value;
};

// 이동 명령 시점에 계산한 경로 (4.5). 마지막 지점이 명령 목표 좌표다.
// 도착하면 컴포넌트가 지워진다.
struct MovePath
{
    std::vector<mapdata::Pos> waypoints;
    std::size_t next = 0;
};

}
