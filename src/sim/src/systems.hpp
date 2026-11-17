#pragma once

#include "sim/sim_state.hpp"

// sim 내부 시스템. Step만 부른다 (시스템끼리는 서로 부르지 않는다, 0장 원칙 2).
namespace sim
{

void ApplyCommands(SimState& state);
void Movement(SimState& state);

}
