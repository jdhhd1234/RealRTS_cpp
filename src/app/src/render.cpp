#include "render.hpp"

#include <cstdint>

namespace app
{

namespace
{

constexpr float kUnitHalfWidth = 9.0f;  // px
constexpr float kUnitHalfHeight = 6.0f; // px
constexpr float kPickRadius = 12.0f;    // px

void SetColor(SDL_Renderer* renderer, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

SDL_Color TerrainColor(mapdata::Terrain terrain)
{
    switch (terrain)
    {
    case mapdata::Terrain::Plains: return SDL_Color{196, 206, 160, 255};
    case mapdata::Terrain::Forest: return SDL_Color{98, 138, 86, 255};
    case mapdata::Terrain::Marsh: return SDL_Color{126, 148, 128, 255};
    case mapdata::Terrain::Urban: return SDL_Color{160, 160, 160, 255};
    case mapdata::Terrain::Mountain: return SDL_Color{156, 126, 96, 255};
    case mapdata::Terrain::Water: return SDL_Color{84, 136, 204, 255};
    }
    return SDL_Color{255, 0, 255, 255};
}

// 12.5 개념도 색: 파랑 #3574c1, 빨강 #c95049.
SDL_Color SideColor(mapdata::Side side)
{
    return side == 0 ? SDL_Color{0x35, 0x74, 0xc1, 255} : SDL_Color{0xc9, 0x50, 0x49, 255};
}

SDL_Color EdgeColor(mapdata::EdgeKind kind)
{
    switch (kind)
    {
    case mapdata::EdgeKind::Rail: return SDL_Color{40, 40, 40, 255};
    case mapdata::EdgeKind::Road: return SDL_Color{150, 100, 50, 255};
    case mapdata::EdgeKind::River: return SDL_Color{40, 90, 170, 255};
    }
    return SDL_Color{255, 0, 255, 255};
}

}

void DrawMap(SDL_Renderer* renderer, const Camera2D& camera, SDL_FPoint viewSize, const mapdata::Map& map)
{
    const mapdata::CellGrid& grid = map.grid;
    const float cellPixels = grid.cellSize * camera.pixelsPerMeter;

    // shortcut: 셀마다 사각형을 그린다. 큰 지도(수백만 셀)에서는 6장대로 텍스처 하나로 구워서 그린다.
    for (std::uint32_t y = 0; y < grid.height; ++y)
    {
        for (std::uint32_t x = 0; x < grid.width; ++x)
        {
            const mapdata::Pos topLeft{static_cast<float>(x) * grid.cellSize, static_cast<float>(y + 1) * grid.cellSize};
            const SDL_FPoint screen = WorldToScreen(camera, viewSize, topLeft);
            const SDL_FRect rect{screen.x, screen.y, cellPixels + 0.5f, cellPixels + 0.5f};
            SetColor(renderer, TerrainColor(grid.terrain[mapdata::CellIndex(grid, x, y)]));
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    for (const mapdata::Edge& edge : map.edges)
    {
        const SDL_FPoint a = WorldToScreen(camera, viewSize, map.nodes[mapdata::ToIndex(edge.from)].pos);
        const SDL_FPoint b = WorldToScreen(camera, viewSize, map.nodes[mapdata::ToIndex(edge.to)].pos);
        // 철도와 도로가 겹쳐도 보이게 도로는 살짝 아래로 띄운다.
        const float offset = edge.kind == mapdata::EdgeKind::Road ? 3.0f : 0.0f;
        SetColor(renderer, EdgeColor(edge.kind));
        SDL_RenderLine(renderer, a.x, a.y + offset, b.x, b.y + offset);
    }

    SetColor(renderer, SDL_Color{40, 40, 40, 255});
    for (const mapdata::Node& node : map.nodes)
    {
        const SDL_FPoint p = WorldToScreen(camera, viewSize, node.pos);
        const SDL_FRect rect{p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void DrawUnits(SDL_Renderer* renderer, const Camera2D& camera, SDL_FPoint viewSize, const sim::SimState& state,
               std::optional<sim::UnitId> selected)
{
    for (auto [entity, info, position] : state.registry.view<const sim::UnitInfo, const sim::Position>().each())
    {
        const SDL_FPoint p = WorldToScreen(camera, viewSize, position.value);
        const SDL_Color color = SideColor(info.side);

        if (const auto* path = state.registry.try_get<sim::MovePath>(entity))
        {
            SetColor(renderer, color);
            SDL_FPoint from = p;
            for (std::size_t i = path->next; i < path->waypoints.size(); ++i)
            {
                const SDL_FPoint to = WorldToScreen(camera, viewSize, path->waypoints[i]);
                SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);
                from = to;
            }
        }

        const SDL_FRect body{p.x - kUnitHalfWidth, p.y - kUnitHalfHeight, kUnitHalfWidth * 2.0f, kUnitHalfHeight * 2.0f};
        SetColor(renderer, color);
        SDL_RenderFillRect(renderer, &body);
        SetColor(renderer, SDL_Color{20, 20, 20, 255});
        SDL_RenderRect(renderer, &body);

        if (selected && *selected == info.id)
        {
            SetColor(renderer, SDL_Color{255, 220, 0, 255});
            for (float grow = 2.0f; grow <= 3.0f; grow += 1.0f)
            {
                const SDL_FRect ring{body.x - grow, body.y - grow, body.w + grow * 2.0f, body.h + grow * 2.0f};
                SDL_RenderRect(renderer, &ring);
            }
        }
    }
}

std::optional<sim::UnitId> PickUnit(const sim::SimState& state, const Camera2D& camera, SDL_FPoint viewSize, SDL_FPoint screen)
{
    std::optional<sim::UnitId> best;
    float bestDistanceSquared = kPickRadius * kPickRadius;
    for (auto [entity, info, position] : state.registry.view<const sim::UnitInfo, const sim::Position>().each())
    {
        const SDL_FPoint p = WorldToScreen(camera, viewSize, position.value);
        const float dx = p.x - screen.x;
        const float dy = p.y - screen.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared <= bestDistanceSquared)
        {
            bestDistanceSquared = distanceSquared;
            best = info.id;
        }
    }
    return best;
}

}
