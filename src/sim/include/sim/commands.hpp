#pragma once

#include <variant>

#include "mapdata/map.hpp"
#include "sim/components.hpp"

namespace sim
{

struct SimState;

// 모든 조작은 명령으로만 코어에 들어간다 (4.11). 플레이어와 AI가 같은 명령을 쓴다.
namespace cmd
{
struct Move
{
    UnitId unit{};
    mapdata::Pos goal;
};
}

using SimCommand = std::variant<cmd::Move>;

// pending에 쌓는다. 다음 Step의 명령 적용 단계에서 처리된다.
void Submit(SimState& state, SimCommand command);

}
