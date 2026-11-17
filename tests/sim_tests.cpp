#include <catch2/catch_test_macros.hpp>

#include "sim/sim_state.hpp"
#include "sim/tick_pacer.hpp"

TEST_CASE("Step advances tick by one")
{
    sim::SimState state;
    for (int i = 0; i < 3; ++i)
    {
        sim::Step(state);
    }
    REQUIRE(state.clock.tick == 3);
}

TEST_CASE("cadence: system run counts over one game day (1440 ticks)")
{
    sim::SimState state;
    int every1 = 0, every5 = 0, every60 = 0, every360 = 0, every1440 = 0;
    for (int i = 0; i < 1440; ++i)
    {
        every1 += state.clock.Every(1);
        every5 += state.clock.Every(5);
        every60 += state.clock.Every(60);
        every360 += state.clock.Every(360);
        every1440 += state.clock.Every(1440);
        sim::Step(state);
    }
    REQUIRE(every1 == 1440);  // 명령·이동
    REQUIRE(every5 == 288);   // 전투
    REQUIRE(every60 == 24);   // 보급
    REQUIRE(every360 == 4);   // 파르티잔
    REQUIRE(every1440 == 1);  // 경제
}

TEST_CASE("TickPacer: 1x runs one tick per real second")
{
    sim::TickPacer pacer;
    REQUIRE(pacer.Advance(0.5) == 0);
    REQUIRE(pacer.Advance(0.5) == 1);
}

TEST_CASE("TickPacer: speed multiplies ticks, 0 pauses")
{
    sim::TickPacer pacer;
    pacer.SetSpeed(10);
    REQUIRE(pacer.Advance(1.0) == 10);
    pacer.SetSpeed(60);
    REQUIRE(pacer.Advance(0.5) == 30);
    pacer.SetSpeed(0);
    REQUIRE(pacer.Advance(5.0) == 0);
}

TEST_CASE("TickPacer: backlog capped at one second of ticks")
{
    sim::TickPacer pacer;
    pacer.SetSpeed(2);
    REQUIRE(pacer.Advance(5.0) == 2);
    REQUIRE(pacer.Advance(0.0) == 0);
}

TEST_CASE("same tick count gives same clock regardless of speed")
{
    sim::SimState slow, fast;
    sim::TickPacer slowPacer, fastPacer;
    fastPacer.SetSpeed(60);

    int slowTicks = 0;
    for (int frame = 0; frame < 240; ++frame) // 1x로 60초, 0.25초 프레임 (2진수로 정확한 값)
    {
        slowTicks += slowPacer.Advance(0.25);
    }
    int fastTicks = fastPacer.Advance(1.0); // 60x로 1초

    for (int i = 0; i < slowTicks; ++i) sim::Step(slow);
    for (int i = 0; i < fastTicks; ++i) sim::Step(fast);
    REQUIRE(slowTicks == 60);
    REQUIRE(slow.clock.tick == fast.clock.tick);
}
