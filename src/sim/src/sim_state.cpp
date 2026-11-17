#include "sim/sim_state.hpp"

namespace sim
{

void Step(SimState& state)
{
    ++state.clock.tick;
}

}
