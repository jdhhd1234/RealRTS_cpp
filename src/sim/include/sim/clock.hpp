#pragma once

#include <cstdint>

namespace sim
{

// 1 tick = 게임 1분 (4.2). tick 크기는 배속과 무관하게 고정.
struct SimClock
{
    std::uint64_t tick = 0;

    // 시스템 실행 조건: tick % cadence == 0. tick 0에서는 모든 시스템이 돈다.
    constexpr bool Every(std::uint64_t cadence) const
    {
        return tick % cadence == 0;
    }
};

}
