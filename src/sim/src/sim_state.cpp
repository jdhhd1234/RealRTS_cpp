#include "sim/sim_state.hpp"

#include "systems.hpp"

namespace sim
{

SimState LoadScenario(const mapdata::Baked& baked)
{
    mapdata::Validate(baked);

    SimState state;
    state.map = baked.map;
    for (std::size_t i = 0; i < baked.scenario.units.size(); ++i)
    {
        const mapdata::UnitSpawn& spawn = baked.scenario.units[i];
        const entt::entity entity = state.registry.create();
        state.registry.emplace<UnitInfo>(entity, UnitId{static_cast<std::uint32_t>(i)}, spawn.side, spawn.name);
        state.registry.emplace<Position>(entity, spawn.pos);
        state.units.push_back(entity);
    }
    return state;
}

void Step(SimState& state)
{
    ApplyCommands(state);
    Movement(state);
    ++state.clock.tick;
}

}
