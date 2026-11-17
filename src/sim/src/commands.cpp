#include "sim/commands.hpp"

#include "sim/pathfinding.hpp"
#include "systems.hpp"

namespace sim
{

namespace
{

void Apply(SimState& state, const cmd::Move& move)
{
    const auto index = static_cast<std::size_t>(move.unit);
    if (index >= state.units.size() || !state.registry.valid(state.units[index]))
    {
        return; // 없는 부대: 무시
    }

    const entt::entity entity = state.units[index];
    const mapdata::Pos from = state.registry.get<Position>(entity).value;
    std::optional<std::vector<mapdata::Pos>> path = FindPath(state.map.grid, from, move.goal);
    if (!path)
    {
        return; // 갈 수 없는 목표: 기존 명령 유지
    }
    state.registry.emplace_or_replace<MovePath>(entity, std::move(*path), std::size_t{0});
}

}

void Submit(SimState& state, SimCommand command)
{
    state.pending.push_back(std::move(command));
}

void ApplyCommands(SimState& state)
{
    for (const SimCommand& command : state.pending)
    {
        std::visit([&state](const auto& c) { Apply(state, c); }, command);
    }
    state.pending.clear();
}

}
