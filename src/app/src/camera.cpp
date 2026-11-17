#include "camera.hpp"

#include <algorithm>

namespace app
{

SDL_FPoint WorldToScreen(const Camera2D& camera, SDL_FPoint viewSize, mapdata::Pos world)
{
    return SDL_FPoint{
        (world.x - camera.centerX) * camera.pixelsPerMeter + viewSize.x * 0.5f,
        viewSize.y * 0.5f - (world.y - camera.centerY) * camera.pixelsPerMeter,
    };
}

mapdata::Pos ScreenToWorld(const Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen)
{
    return mapdata::Pos{
        (screen.x - viewSize.x * 0.5f) / camera.pixelsPerMeter + camera.centerX,
        (viewSize.y * 0.5f - screen.y) / camera.pixelsPerMeter + camera.centerY,
    };
}

Camera2D FitToMap(const mapdata::CellGrid& grid, SDL_FPoint viewSize)
{
    const float worldWidth = static_cast<float>(grid.width) * grid.cellSize;
    const float worldHeight = static_cast<float>(grid.height) * grid.cellSize;
    const float margin = 0.95f;
    return Camera2D{
        worldWidth * 0.5f,
        worldHeight * 0.5f,
        std::min(viewSize.x / worldWidth, viewSize.y / worldHeight) * margin,
    };
}

void ZoomAt(Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen, float factor)
{
    const mapdata::Pos before = ScreenToWorld(camera, viewSize, screen);
    camera.pixelsPerMeter = std::clamp(camera.pixelsPerMeter * factor, 0.0001f, 1.0f);
    const mapdata::Pos after = ScreenToWorld(camera, viewSize, screen);
    camera.centerX += before.x - after.x;
    camera.centerY += before.y - after.y;
}

void Pan(Camera2D& camera, float screenDx, float screenDy)
{
    camera.centerX -= screenDx / camera.pixelsPerMeter;
    camera.centerY += screenDy / camera.pixelsPerMeter;
}

}
