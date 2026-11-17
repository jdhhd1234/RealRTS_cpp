#pragma once

#include "sim/clock.hpp"

namespace sim
{

    struct SimState
    {
        SimClock clock;
    };

    // 1 tick 진행. 시스템은 이후 마일스톤에서 clock.Every(cadence) 조건으로 여기에 추가한다 (4.2).
    void Step(SimState& state);

}
