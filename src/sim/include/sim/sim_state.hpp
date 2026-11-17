#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "mapdata/map.hpp"
#include "sim/clock.hpp"
#include "sim/commands.hpp"
#include "sim/components.hpp"
#include "sim/supply.hpp"

namespace sim
{

// 시뮬레이션 상태 전체 (4.1). app은 const로만 읽고, 쓰기는 Submit으로만 한다.
struct SimState
{
    entt::registry registry;
    SimClock clock;
    mapdata::Map map;
    SupplyState supply;
    std::vector<entt::entity> units; // UnitId 인덱스 -> 엔티티
    std::vector<SimCommand> pending; // 다음 Step에 적용할 명령
};

// 시나리오 시작 상태(tick 0)를 만든다. 지도가 맞지 않으면 mapdata::LoadError.
SimState LoadScenario(const mapdata::Baked& baked);

// 1 tick 진행 (4.2). 순서: 명령 적용 -> 소모 -> 이동 -> 수송 도착 -> (1시간마다) 보급 배분.
void Step(SimState& state);

// 기지 + 보급소 + 파이프라인 + 사단이 가진 물자 합계 (4.7 보존 검사용).
Cargo SupplyOnHand(const SimState& state);

}
