#include <cmath>

#include "sim/terrain.hpp"
#include "systems.hpp"

namespace sim
{

namespace
{

float CostAt(const mapdata::CellGrid& grid, mapdata::Pos pos)
{
    const std::optional<std::size_t> cell = mapdata::CellAt(grid, pos);
    if (!cell)
    {
        return 1.0f;
    }
    const float cost = TerrainCost(grid.terrain[*cell]);
    return std::isfinite(cost) ? cost : 1.0f; // 물 위에서 시작한 부대는 평지 속도로 빠져나간다
}

}

void Movement(SimState& state)
{
    std::vector<entt::entity> arrived;

    for (auto [entity, position, path] : state.registry.view<Position, MovePath>().each())
    {
        // 이번 tick 이동 예산. 지형 비용은 현재 서 있는 셀 기준으로 적용한다.
        float budget = kMarchMetersPerTick;
        while (budget > 0.0f && path.next < path.waypoints.size())
        {
            const mapdata::Pos target = path.waypoints[path.next];
            const float dx = target.x - position.value.x;
            const float dy = target.y - position.value.y;
            const float distance = std::sqrt(dx * dx + dy * dy); // sqrt는 IEEE 정확 반올림이라 플랫폼 간 같다
            const float cost = CostAt(state.map.grid, position.value);
            const float reach = budget / cost;

            if (distance <= reach)
            {
                position.value = target;
                budget -= distance * cost;
                ++path.next;
            }
            else
            {
                position.value.x += dx / distance * reach;
                position.value.y += dy / distance * reach;
                budget = 0.0f;
            }
        }

        if (path.next >= path.waypoints.size())
        {
            arrived.push_back(entity);
        }
    }

    for (const entt::entity entity : arrived)
    {
        state.registry.remove<MovePath>(entity);
    }
}

}
