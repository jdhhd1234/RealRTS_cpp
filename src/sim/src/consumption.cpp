#include <algorithm>

#include "systems.hpp"

namespace sim
{

// 매 tick 일일 소모율의 1/1440을 쓴다. 이동 명령이 있으면 행군 소모율 (4.4 Consumption).
// 재고 0 아래로는 내려가지 않는다. 재고 0의 효과(이동 불가 등)는 M5.
void Consumption(SimState& state)
{
    for (auto [entity, stocks] : state.registry.view<Stocks>().each())
    {
        const Cargo& perDay = state.registry.all_of<MovePath>(entity) ? kMarchUsePerDay : kIdleUsePerDay;
        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            const double used = std::min(stocks.amount[g], perDay[g] / 1440.0);
            stocks.amount[g] -= used;
            state.supply.consumed[g] += used;
        }
    }
}

}
