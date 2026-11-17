#include "sim/pathfinding.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <queue>

#include "sim/terrain.hpp"

namespace sim
{

namespace
{

constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr std::size_t kNoCell = std::numeric_limits<std::size_t>::max();
constexpr float kDiagonal = 1.41421356f;

struct OpenEntry
{
    float estimate = 0.0f; // 지금까지 비용 + 휴리스틱
    std::size_t cell = 0;
};

// 옥타일 거리 (셀 단위). 최소 지형 비용이 1이므로 과대평가하지 않는다.
float Heuristic(const mapdata::CellGrid& grid, std::size_t from, std::size_t to)
{
    const auto fx = static_cast<std::int64_t>(from % grid.width);
    const auto fy = static_cast<std::int64_t>(from / grid.width);
    const auto tx = static_cast<std::int64_t>(to % grid.width);
    const auto ty = static_cast<std::int64_t>(to / grid.width);
    const auto dx = static_cast<float>(std::abs(fx - tx));
    const auto dy = static_cast<float>(std::abs(fy - ty));
    return std::max(dx, dy) + (kDiagonal - 1.0f) * std::min(dx, dy);
}

}

std::optional<std::vector<mapdata::Pos>> FindPath(const mapdata::CellGrid& grid, mapdata::Pos from, mapdata::Pos goal)
{
    const std::optional<std::size_t> startCell = mapdata::CellAt(grid, from);
    const std::optional<std::size_t> goalCell = mapdata::CellAt(grid, goal);
    if (!startCell || !goalCell || !IsPassable(grid.terrain[*goalCell]))
    {
        return std::nullopt;
    }
    if (*startCell == *goalCell)
    {
        return std::vector<mapdata::Pos>{goal};
    }

    // shortcut: 호출마다 셀 수만큼 배열을 잡는다 (2000x2000이면 약 32 MB). 명령이 잦아지면 재사용 버퍼로 바꾼다.
    std::vector<float> cost(grid.terrain.size(), kInfinity);
    std::vector<std::size_t> cameFrom(grid.terrain.size(), kNoCell);

    // 추정 비용이 같으면 셀 인덱스가 작은 쪽 먼저 (결정론).
    const auto later = [](const OpenEntry& a, const OpenEntry& b)
    {
        return a.estimate != b.estimate ? a.estimate > b.estimate : a.cell > b.cell;
    };
    std::priority_queue<OpenEntry, std::vector<OpenEntry>, decltype(later)> open(later);

    cost[*startCell] = 0.0f;
    open.push(OpenEntry{Heuristic(grid, *startCell, *goalCell), *startCell});

    const auto passableAt = [&grid](std::int64_t x, std::int64_t y)
    {
        const std::size_t cell = mapdata::CellIndex(grid, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
        return IsPassable(grid.terrain[cell]);
    };
    const auto width = static_cast<std::int64_t>(grid.width);
    const auto height = static_cast<std::int64_t>(grid.height);

    while (!open.empty())
    {
        const OpenEntry current = open.top();
        open.pop();
        if (current.cell == *goalCell)
        {
            break;
        }
        if (current.estimate > cost[current.cell] + Heuristic(grid, current.cell, *goalCell))
        {
            continue; // 더 싼 경로로 이미 갱신된 낡은 항목
        }

        const auto cx = static_cast<std::int64_t>(current.cell % grid.width);
        const auto cy = static_cast<std::int64_t>(current.cell / grid.width);
        for (std::int64_t dy = -1; dy <= 1; ++dy)
        {
            for (std::int64_t dx = -1; dx <= 1; ++dx)
            {
                const std::int64_t nx = cx + dx;
                const std::int64_t ny = cy + dy;
                if ((dx == 0 && dy == 0) || nx < 0 || ny < 0 || nx >= width || ny >= height)
                {
                    continue;
                }
                if (!passableAt(nx, ny))
                {
                    continue;
                }
                const bool diagonal = dx != 0 && dy != 0;
                if (diagonal && (!passableAt(cx + dx, cy) || !passableAt(cx, cy + dy)))
                {
                    continue; // 물 모서리를 대각선으로 가로지르지 않는다
                }

                const std::size_t next = mapdata::CellIndex(grid, static_cast<std::uint32_t>(nx), static_cast<std::uint32_t>(ny));
                const float step = (diagonal ? kDiagonal : 1.0f) * TerrainCost(grid.terrain[next]);
                const float newCost = cost[current.cell] + step;
                if (newCost < cost[next])
                {
                    cost[next] = newCost;
                    cameFrom[next] = current.cell;
                    open.push(OpenEntry{newCost + Heuristic(grid, next, *goalCell), next});
                }
            }
        }
    }

    if (cost[*goalCell] == kInfinity)
    {
        return std::nullopt;
    }

    std::vector<mapdata::Pos> waypoints;
    for (std::size_t cell = *goalCell; cell != *startCell; cell = cameFrom[cell])
    {
        waypoints.push_back(mapdata::CellCenter(grid, cell));
    }
    std::reverse(waypoints.begin(), waypoints.end());
    waypoints.back() = goal;
    return waypoints;
}

}
