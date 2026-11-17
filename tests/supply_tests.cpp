#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "mapdata/test_map.hpp"
#include "sim/sim_state.hpp"

using Catch::Matchers::WithinAbs;

namespace
{

mapdata::Pos Center(std::uint32_t x, std::uint32_t y)
{
    return mapdata::Pos{(static_cast<float>(x) + 0.5f) * 1000.0f, (static_cast<float>(y) + 0.5f) * 1000.0f};
}

sim::Cargo& StocksOf(sim::SimState& state, std::uint32_t unit)
{
    return state.registry.get<sim::Stocks>(state.units[unit]).amount;
}

void Run(sim::SimState& state, int ticks)
{
    for (int i = 0; i < ticks; ++i)
    {
        sim::Step(state);
    }
}

constexpr std::size_t kFuel = sim::GoodIndex(sim::Good::Fuel);
constexpr std::size_t kFood = sim::GoodIndex(sim::Good::Food);

}

TEST_CASE("supply starts full: bases 20000 t, depots and units at target")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    REQUIRE(state.supply.bases.size() == 2);
    REQUIRE(state.supply.depots.size() == 2);
    REQUIRE(state.supply.bases[0].stock[kFuel] == 20000.0);
    REQUIRE(state.supply.depots[1].stock[kFood] == sim::kDepotTargetStock);
    REQUIRE(StocksOf(state, 0)[kFuel] == sim::kUnitTargetStock[kFuel]);
}

TEST_CASE("invariant: supply is conserved (start = on hand + consumed)")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    state.supply.depots[0].stock = sim::Cargo{}; // 수송이 실제로 일어나게 비운다
    const sim::Cargo start = sim::SupplyOnHand(state);

    sim::Submit(state, sim::cmd::Move{sim::UnitId{0}, Center(20, 3)});
    sim::Submit(state, sim::cmd::Move{sim::UnitId{1}, Center(3, 14)});
    for (int hour = 0; hour < 72; ++hour)
    {
        Run(state, 60);
        const sim::Cargo now = sim::SupplyOnHand(state);
        for (std::size_t g = 0; g < sim::kGoodCount; ++g)
        {
            REQUIRE_THAT(now[g] + state.supply.consumed[g], WithinAbs(start[g], 1e-6));
        }
    }
    REQUIRE(state.supply.consumed[kFuel] > 0.0);
}

TEST_CASE("invariant: no edge carries more than its hourly capacity")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    for (sim::SupplyPoint& depot : state.supply.depots)
    {
        depot.stock = sim::Cargo{}; // 보급소 2곳 x 3품목 x 2000 t 수요 >> 철도 125 t/시간
    }

    bool saturated = false;
    for (int hour = 0; hour < 48; ++hour)
    {
        Run(state, 60); // 첫 tick이 보급 단계
        for (std::size_t e = 0; e < state.map.edges.size(); ++e)
        {
            const double capacity = state.map.edges[e].capacity / 24.0;
            REQUIRE(state.supply.edgeFlowThisHour[e] <= capacity + 1e-9);
            saturated = saturated || state.supply.edgeFlowThisHour[e] >= capacity - 1e-9;
        }
    }
    REQUIRE(saturated); // 용량 제약이 실제로 걸렸다
}

TEST_CASE("shipments take transit time before reaching the depot")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    state.supply.depots[0].stock = sim::Cargo{};

    Run(state, 1); // tick 0: 보급 단계에서 기지 -> 보급소 발송
    REQUIRE_FALSE(state.supply.pipeline.empty());
    REQUIRE(state.supply.depots[0].stock[kFuel] == 0.0);

    Run(state, 30); // 철도 6 km / 25 km/h = 15분
    REQUIRE(state.supply.depots[0].stock[kFuel] > 0.0);
}

TEST_CASE("invariant: isolated unit gets no supply")
{
    mapdata::Baked baked = mapdata::MakeTestMap();
    baked.scenario.depots.pop_back(); // Red 보급소 제거 -> Red 사단 고립
    sim::SimState state = sim::LoadScenario(baked);

    double previousFood = StocksOf(state, 1)[kFood];
    for (int hour = 0; hour < 48; ++hour)
    {
        Run(state, 60);
        REQUIRE(StocksOf(state, 1)[kFood] < previousFood);
        previousFood = StocksOf(state, 1)[kFood];
    }
    REQUIRE_FALSE(state.registry.get<sim::SupplySource>(state.units[1]).depotNode.has_value());
    REQUIRE(state.registry.get<sim::SupplySource>(state.units[0]).depotNode == mapdata::NodeId{1});
    // Blue는 매시간 채워진다. 마지막 보급 뒤 59 tick 동안 먹은 양(약 1.23 t)만큼만 모자란다.
    REQUIRE_THAT(StocksOf(state, 0)[kFood], WithinAbs(sim::kUnitTargetStock[kFood], 2.0));
}

TEST_CASE("marching burns fuel ten times faster than idling")
{
    mapdata::Baked baked = mapdata::MakeTestMap();
    baked.scenario.depots.clear(); // 보급 없이 소모만 본다
    sim::SimState state = sim::LoadScenario(baked);

    sim::Submit(state, sim::cmd::Move{sim::UnitId{0}, Center(10, 0)});
    Run(state, 10); // 6 km 남았으므로 10 tick 동안 계속 행군
    const double marchUsed = sim::kUnitTargetStock[kFuel] - StocksOf(state, 0)[kFuel];
    const double idleUsed = sim::kUnitTargetStock[kFuel] - StocksOf(state, 1)[kFuel];
    REQUIRE_THAT(marchUsed, WithinAbs(200.0 / 1440.0 * 10.0, 1e-9));
    REQUIRE_THAT(idleUsed, WithinAbs(20.0 / 1440.0 * 10.0, 1e-9));
}

TEST_CASE("truck efficiency falls with distance from depot")
{
    REQUIRE(sim::TruckEfficiency(10000.0f) == 1.0);
    REQUIRE_THAT(sim::TruckEfficiency(60000.0f), WithinAbs(0.5, 1e-9));
    REQUIRE(sim::TruckEfficiency(100000.0f) == 0.0);
    REQUIRE(sim::TruckEfficiency(150000.0f) == 0.0);
}
