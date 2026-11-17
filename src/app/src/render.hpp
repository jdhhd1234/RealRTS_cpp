#pragma once

#include <optional>

#include <SDL3/SDL.h>

#include "camera.hpp"
#include "sim/sim_state.hpp"

namespace app
{

// M3 임시 화면: 지형 셀 색, 수송망 선, 부대는 진영 색 사각형.
void DrawMap(SDL_Renderer* renderer, const Camera2D& camera, SDL_FPoint viewSize, const mapdata::Map& map);
void DrawUnits(SDL_Renderer* renderer, const Camera2D& camera, SDL_FPoint viewSize, const sim::SimState& state,
               std::optional<sim::UnitId> selected);

// 화면 좌표에서 가장 가까운 부대 (반경 안에 없으면 nullopt).
std::optional<sim::UnitId> PickUnit(const sim::SimState& state, const Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen);

}
