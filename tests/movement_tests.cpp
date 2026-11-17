#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "mapdata/test_map.hpp"
#include "sim/pathfinding.hpp"
#include "sim/sim_state.hpp"

namespace
{

mapdata::Pos Center(std::uint32_t x, std::uint32_t y)
{
    return mapdata::Pos{(static_cast<float>(x) + 0.5f) * 1000.0f, (static_cast<float>(y) + 0.5f) * 1000.0f};
}

mapdata::Pos& PositionOf(sim::SimState& state, std::uint32_t unit)
{
    return state.registry.get<sim::Position>(state.units[unit]).value;
}

// 이동이 끝날 때까지 tick 수. 끝나지 않으면 -1.
int TicksUntilArrived(sim::SimState& state, std::uint32_t unit)
{
    for (int tick = 0; tick < 10000; ++tick)
    {
        sim::Step(state);
        if (!state.registry.all_of<sim::MovePath>(state.units[unit]))
        {
            return tick + 1;
        }
    }
    return -1;
}

}

TEST_CASE("LoadScenario creates one entity per scenario unit")
{
    const sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    REQUIRE(state.units.size() == 2);

    const auto& info = state.registry.get<sim::UnitInfo>(state.units[1]);
    REQUIRE(info.id == sim::UnitId{1});
    REQUIRE(info.side == 1);
    REQUIRE(info.name == "1st Red Division");
    REQUIRE(state.registry.get<sim::Position>(state.units[0]).value.x == Center(10, 6).x);
}

TEST_CASE("FindPath goes straight on open plains and ends exactly at goal")
{
    const mapdata::Baked baked = mapdata::MakeTestMap();
    const mapdata::Pos goal{10500.0f, 2100.0f};
    const auto path = sim::FindPath(baked.map.grid, Center(10, 6), goal);

    REQUIRE(path.has_value());
    REQUIRE(path->size() == 4); // 셀 (10,5) (10,4) (10,3) + 목표
    REQUIRE(path->back().x == goal.x);
    REQUIRE(path->back().y == goal.y);
}

TEST_CASE("FindPath crosses the river only at the bridge")
{
    const mapdata::Baked baked = mapdata::MakeTestMap();
    const mapdata::CellGrid& grid = baked.map.grid;
    const auto path = sim::FindPath(grid, Center(10, 6), Center(14, 6));
    REQUIRE(path.has_value());

    bool crossed = false;
    for (const mapdata::Pos& point : *path)
    {
        const std::size_t cell = *mapdata::CellAt(grid, point);
        if (cell % grid.width == 12)
        {
            REQUIRE(cell / grid.width == 8);
            crossed = true;
        }
    }
    REQUIRE(crossed);
}

TEST_CASE("FindPath rejects water and off-map goals")
{
    const mapdata::Baked baked = mapdata::MakeTestMap();
    REQUIRE_FALSE(sim::FindPath(baked.map.grid, Center(10, 6), Center(12, 2)).has_value());
    REQUIRE_FALSE(sim::FindPath(baked.map.grid, Center(10, 6), mapdata::Pos{-5.0f, 100.0f}).has_value());
}

TEST_CASE("Move command walks the unit to the goal over several ticks")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    const mapdata::Pos start = PositionOf(state, 0);
    const mapdata::Pos goal = Center(10, 1);
    sim::Submit(state, sim::cmd::Move{sim::UnitId{0}, goal});

    sim::Step(state);
    const mapdata::Pos afterOne = PositionOf(state, 0);
    const float moved = std::sqrt((afterOne.x - start.x) * (afterOne.x - start.x) + (afterOne.y - start.y) * (afterOne.y - start.y));
    REQUIRE(moved > 0.0f);
    REQUIRE(moved <= 300.0f + 0.01f);

    REQUIRE(TicksUntilArrived(state, 0) > 0);
    REQUIRE(PositionOf(state, 0).x == goal.x);
    REQUIRE(PositionOf(state, 0).y == goal.y);
    REQUIRE(state.pending.empty());
}

TEST_CASE("Move to unreachable goal or unknown unit is ignored")
{
    sim::SimState state = sim::LoadScenario(mapdata::MakeTestMap());
    const mapdata::Pos start = PositionOf(state, 0);
    sim::Submit(state, sim::cmd::Move{sim::UnitId{0}, Center(12, 2)}); // 강 위
    sim::Submit(state, sim::cmd::Move{sim::UnitId{99}, Center(10, 1)});
    sim::Step(state);

    REQUIRE_FALSE(state.registry.all_of<sim::MovePath>(state.units[0]));
    REQUIRE(PositionOf(state, 0).x == start.x);
    REQUIRE(PositionOf(state, 0).y == start.y);
}

TEST_CASE("forest is twice as slow as plains")
{
    sim::SimState plains = sim::LoadScenario(mapdata::MakeTestMap());
    PositionOf(plains, 0) = Center(6, 1);
    sim::Submit(plains, sim::cmd::Move{sim::UnitId{0}, Center(9, 1)});

    sim::SimState forest = sim::LoadScenario(mapdata::MakeTestMap());
    PositionOf(forest, 0) = Center(2, 4);
    sim::Submit(forest, sim::cmd::Move{sim::UnitId{0}, Center(5, 4)});

    REQUIRE(TicksUntilArrived(plains, 0) == 10); // 3 km / 300 m
    REQUIRE(TicksUntilArrived(forest, 0) == 20);
}

TEST_CASE("movement is deterministic for the same commands")
{
    sim::SimState a = sim::LoadScenario(mapdata::MakeTestMap());
    sim::SimState b = sim::LoadScenario(mapdata::MakeTestMap());
    for (sim::SimState* state : {&a, &b})
    {
        sim::Submit(*state, sim::cmd::Move{sim::UnitId{0}, Center(20, 3)});
        sim::Submit(*state, sim::cmd::Move{sim::UnitId{1}, Center(3, 14)});
        for (int i = 0; i < 37; ++i)
        {
            sim::Step(*state);
        }
    }
    for (std::uint32_t unit = 0; unit < 2; ++unit)
    {
        REQUIRE(PositionOf(a, unit).x == PositionOf(b, unit).x);
        REQUIRE(PositionOf(a, unit).y == PositionOf(b, unit).y);
    }
}
