#pragma once

#include <SDL3/SDL.h>

#include "mapdata/map.hpp"

namespace app
{

// 2D 직교 카메라 (6장). 월드 좌표는 y 북쪽+, 화면 좌표는 y 아래쪽+ 이라서 변환에서 y를 뒤집는다 (5.1).
struct Camera2D
{
    float centerX = 0.0f; // 화면 중앙의 월드 좌표 (m)
    float centerY = 0.0f;
    float pixelsPerMeter = 0.05f;
};

SDL_FPoint WorldToScreen(const Camera2D& camera, SDL_FPoint viewSize, mapdata::Pos world);
mapdata::Pos ScreenToWorld(const Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen);

// 지도 전체가 화면에 들어오게 맞춘다.
Camera2D FitToMap(const mapdata::CellGrid& grid, SDL_FPoint viewSize);

// 커서 아래 지점이 그대로 있도록 확대/축소한다.
void ZoomAt(Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen, float factor);

// 화면 픽셀만큼 끌기.
void Pan(Camera2D& camera, float screenDx, float screenDy);

}
