#include "sim/tick_pacer.hpp"

#include <algorithm>

namespace sim
{

void TickPacer::SetSpeed(int ticksPerSecond)
{
    speed = std::max(ticksPerSecond, 0);
}

int TickPacer::Speed() const
{
    return speed;
}

int TickPacer::Advance(double realSeconds)
{
    // 밀린 tick은 최대 1초 분량까지만 따라잡는다. 창 드래그 같은 멈춤 뒤에 한꺼번에 몰아 돌지 않게.
    const double maxPending = std::max(speed, 1);
    pendingTicks = std::min(pendingTicks + realSeconds * speed, maxPending);

    const int ticks = static_cast<int>(pendingTicks);
    pendingTicks -= ticks;
    return ticks;
}

}
