#include "sim/sim_state.hpp"

#include "systems.hpp"

namespace sim
{

namespace
{

std::vector<SupplyPoint> MakeSupplyPoints(const std::vector<mapdata::Facility>& facilities, double startStock)
{
    std::vector<SupplyPoint> points;
    for (const mapdata::Facility& facility : facilities)
    {
        points.push_back(SupplyPoint{facility.node, facility.side, Cargo{startStock, startStock, startStock}});
    }
    return points;
}

void AddInto(Cargo& total, const Cargo& add)
{
    for (std::size_t g = 0; g < kGoodCount; ++g)
    {
        total[g] += add[g];
    }
}

}

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
        state.registry.emplace<Stocks>(entity, kUnitTargetStock);
        state.registry.emplace<SupplySource>(entity);
        state.units.push_back(entity);
    }

    state.supply.bases = MakeSupplyPoints(baked.scenario.bases, kBaseStartStock);
    state.supply.depots = MakeSupplyPoints(baked.scenario.depots, kDepotTargetStock);
    state.supply.edgeFlowThisHour.assign(state.map.edges.size(), 0.0);
    return state;
}

void Step(SimState& state)
{
    ApplyCommands(state);
    Consumption(state);
    Movement(state);
    ReceiveShipments(state);
    if (state.clock.Every(60))
    {
        Supply(state);
    }
    ++state.clock.tick;
}

Cargo SupplyOnHand(const SimState& state)
{
    Cargo total{};
    for (const SupplyPoint& base : state.supply.bases)
    {
        AddInto(total, base.stock);
    }
    for (const SupplyPoint& depot : state.supply.depots)
    {
        AddInto(total, depot.stock);
    }
    for (const Shipment& shipment : state.supply.pipeline)
    {
        AddInto(total, shipment.cargo);
    }
    for (const entt::entity entity : state.units)
    {
        if (state.registry.valid(entity))
        {
            AddInto(total, state.registry.get<Stocks>(entity).amount);
        }
    }
    return total;
}

}
