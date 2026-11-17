#include "sim/supply.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <queue>

#include "systems.hpp"

namespace sim
{

namespace
{

constexpr double kEpsilon = 1e-6; // t, 이보다 작은 양은 0으로 본다
constexpr int kMaxRoutesPerDepot = 64; // 무한 반복 방지

double HourlyCapacity(const mapdata::Edge& edge)
{
    // 유효 용량 = capacity x (1 - damage), 적 통제 시 0 (4.7). damage·통제는 M4b/M6에서.
    return static_cast<double>(edge.capacity) / 24.0;
}

double Residual(const SimState& state, std::size_t edge)
{
    return HourlyCapacity(state.map.edges[edge]) - state.supply.edgeFlowThisHour[edge];
}

// 기지에서 꺼낼 수 있는 양: 품목별 min(필요량, 재고).
Cargo Takeable(const Cargo& want, const Cargo& stock)
{
    Cargo take{};
    for (std::size_t g = 0; g < kGoodCount; ++g)
    {
        take[g] = std::max(0.0, std::min(want[g], stock[g]));
    }
    return take;
}

struct Route
{
    std::size_t base = 0;
    std::vector<std::size_t> edges;
    double hours = 0.0;
    double bottleneck = std::numeric_limits<double>::infinity(); // 이번 시간에 더 보낼 수 있는 양 (t)
};

// 보급소에서 수송 시간 기준으로 가장 가까운, 쓸모 있는 재고를 가진 아군 기지 (다익스트라).
// 남은 용량이 없는 간선은 건너뛴다. 같은 시간이면 노드 인덱스가 작은 쪽 먼저 (결정론).
std::optional<Route> NearestBase(const SimState& state, const SupplyPoint& depot, const Cargo& want)
{
    const mapdata::Map& map = state.map;
    const std::size_t nodeCount = map.nodes.size();

    std::vector<std::optional<std::size_t>> baseAt(nodeCount);
    for (std::size_t b = 0; b < state.supply.bases.size(); ++b)
    {
        const SupplyPoint& base = state.supply.bases[b];
        if (base.side == depot.side && Total(Takeable(want, base.stock)) > kEpsilon)
        {
            baseAt[mapdata::ToIndex(base.node)] = b;
        }
    }

    // shortcut: 시간마다 인접 리스트를 새로 만든다. 간선이 수만 개가 되면 SimState에 캐시한다.
    std::vector<std::vector<std::size_t>> edgesAt(nodeCount);
    for (std::size_t e = 0; e < map.edges.size(); ++e)
    {
        edgesAt[mapdata::ToIndex(map.edges[e].from)].push_back(e);
        edgesAt[mapdata::ToIndex(map.edges[e].to)].push_back(e);
    }

    constexpr double kUnreached = std::numeric_limits<double>::infinity();
    constexpr std::size_t kNone = std::numeric_limits<std::size_t>::max();
    std::vector<double> hours(nodeCount, kUnreached);
    std::vector<std::size_t> viaEdge(nodeCount, kNone);

    struct Entry
    {
        double hours = 0.0;
        std::size_t node = 0;
    };
    const auto later = [](const Entry& a, const Entry& b)
    {
        return a.hours != b.hours ? a.hours > b.hours : a.node > b.node;
    };
    std::priority_queue<Entry, std::vector<Entry>, decltype(later)> open(later);

    const std::size_t start = mapdata::ToIndex(depot.node);
    hours[start] = 0.0;
    open.push(Entry{0.0, start});

    while (!open.empty())
    {
        const Entry current = open.top();
        open.pop();
        if (current.hours > hours[current.node])
        {
            continue;
        }

        if (baseAt[current.node])
        {
            Route route;
            route.base = *baseAt[current.node];
            route.hours = current.hours;
            for (std::size_t node = current.node; node != start;)
            {
                const std::size_t e = viaEdge[node];
                route.edges.push_back(e);
                route.bottleneck = std::min(route.bottleneck, Residual(state, e));
                const mapdata::Edge& edge = map.edges[e];
                node = mapdata::ToIndex(edge.from) == node ? mapdata::ToIndex(edge.to) : mapdata::ToIndex(edge.from);
            }
            return route;
        }

        for (const std::size_t e : edgesAt[current.node])
        {
            if (Residual(state, e) <= kEpsilon)
            {
                continue;
            }
            const mapdata::Edge& edge = map.edges[e];
            const std::size_t next = mapdata::ToIndex(edge.from) == current.node ? mapdata::ToIndex(edge.to) : mapdata::ToIndex(edge.from);
            const double nextHours = current.hours + edge.length / TransitMetersPerHour(edge.kind);
            if (nextHours < hours[next])
            {
                hours[next] = nextHours;
                viaEdge[next] = e;
                open.push(Entry{nextHours, next});
            }
        }
    }
    return std::nullopt;
}

// 4.7 3단계: 기지 -> 보급소. 보급소마다 목표 재고까지 모자란 양을, 가까운 기지부터 간선 용량 안에서 보낸다.
void ReplenishDepots(SimState& state)
{
    SupplyState& supply = state.supply;
    for (std::size_t d = 0; d < supply.depots.size(); ++d)
    {
        Cargo want{};
        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            want[g] = kDepotTargetStock - supply.depots[d].stock[g];
        }
        for (const Shipment& shipment : supply.pipeline)
        {
            if (shipment.depot == d)
            {
                for (std::size_t g = 0; g < kGoodCount; ++g)
                {
                    want[g] -= shipment.cargo[g];
                }
            }
        }
        for (double& amount : want)
        {
            amount = std::max(0.0, amount);
        }

        for (int attempt = 0; attempt < kMaxRoutesPerDepot && Total(want) > kEpsilon; ++attempt)
        {
            const std::optional<Route> route = NearestBase(state, supply.depots[d], want);
            if (!route)
            {
                break;
            }

            SupplyPoint& base = supply.bases[route->base];
            Cargo take = Takeable(want, base.stock);
            const double scale = std::min(1.0, route->bottleneck / Total(take));
            for (std::size_t g = 0; g < kGoodCount; ++g)
            {
                take[g] *= scale;
                base.stock[g] -= take[g];
                want[g] -= take[g];
            }
            for (const std::size_t e : route->edges)
            {
                supply.edgeFlowThisHour[e] += Total(take);
            }
            const auto transitTicks = static_cast<std::uint64_t>(std::ceil(route->hours * 60.0));
            supply.pipeline.push_back(Shipment{d, take, state.clock.tick + transitTicks});
        }
    }
}

// 4.7 1·2단계: 보급소 -> 사단. 가장 가까운 아군 보급소에 연결하고, 트럭 거리 효율만큼 요청한다.
// 보급소 재고가 모자라면 요청 비율대로 나눈다 (MVP: 우선순위 없음).
void SupplyUnits(SimState& state)
{
    SupplyState& supply = state.supply;

    struct Request
    {
        entt::entity entity{};
        std::size_t depot = 0;
        Cargo want{};
    };
    std::vector<Request> requests;
    std::vector<Cargo> asked(supply.depots.size(), Cargo{});

    for (const entt::entity entity : state.units) // UnitId 순서 (결정론)
    {
        if (!state.registry.valid(entity))
        {
            continue;
        }
        const UnitInfo& info = state.registry.get<UnitInfo>(entity);
        const mapdata::Pos pos = state.registry.get<Position>(entity).value;
        SupplySource& source = state.registry.get<SupplySource>(entity);

        std::optional<std::size_t> nearest;
        float nearestDistance = std::numeric_limits<float>::infinity();
        for (std::size_t d = 0; d < supply.depots.size(); ++d)
        {
            if (supply.depots[d].side != info.side)
            {
                continue;
            }
            const mapdata::Pos at = state.map.nodes[mapdata::ToIndex(supply.depots[d].node)].pos;
            const float dx = at.x - pos.x;
            const float dy = at.y - pos.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = d;
            }
        }

        const double efficiency = nearest ? TruckEfficiency(nearestDistance) : 0.0;
        if (efficiency <= 0.0)
        {
            source.depotNode.reset(); // 고립: 보급 0
            continue;
        }
        source.depotNode = supply.depots[*nearest].node;

        const Cargo& stock = state.registry.get<Stocks>(entity).amount;
        Cargo want{};
        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            want[g] = std::max(0.0, kUnitTargetStock[g] - stock[g]);
        }
        const double truckLimit = kTruckTonsPerHour * efficiency;
        if (Total(want) > truckLimit)
        {
            const double scale = truckLimit / Total(want);
            for (double& amount : want)
            {
                amount *= scale;
            }
        }

        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            asked[*nearest][g] += want[g];
        }
        requests.push_back(Request{entity, *nearest, want});
    }

    std::vector<Cargo> share(supply.depots.size(), Cargo{});
    for (std::size_t d = 0; d < supply.depots.size(); ++d)
    {
        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            share[d][g] = asked[d][g] > 0.0 ? std::min(1.0, supply.depots[d].stock[g] / asked[d][g]) : 0.0;
        }
    }

    for (const Request& request : requests)
    {
        Cargo& depotStock = supply.depots[request.depot].stock;
        Cargo& unitStock = state.registry.get<Stocks>(request.entity).amount;
        for (std::size_t g = 0; g < kGoodCount; ++g)
        {
            const double given = std::min(request.want[g] * share[request.depot][g], depotStock[g]);
            depotStock[g] -= given;
            unitStock[g] += given;
        }
    }
}

}

double Total(const Cargo& cargo)
{
    double total = 0.0;
    for (const double amount : cargo)
    {
        total += amount;
    }
    return total;
}

double TransitMetersPerHour(mapdata::EdgeKind kind)
{
    switch (kind)
    {
    case mapdata::EdgeKind::Rail: return 25000.0;
    case mapdata::EdgeKind::Road: return 20000.0;
    case mapdata::EdgeKind::River: return 8000.0;
    }
    return 20000.0;
}

double TruckEfficiency(float distanceMeters)
{
    if (distanceMeters <= kTruckFullRange)
    {
        return 1.0;
    }
    const double falloff = static_cast<double>(distanceMeters - kTruckFullRange) / (kTruckMaxRange - kTruckFullRange);
    return std::max(0.0, 1.0 - falloff);
}

void ReceiveShipments(SimState& state)
{
    SupplyState& supply = state.supply;
    const auto arrived = [&state](const Shipment& shipment) { return shipment.arriveTick <= state.clock.tick; };
    for (const Shipment& shipment : supply.pipeline)
    {
        if (arrived(shipment))
        {
            for (std::size_t g = 0; g < kGoodCount; ++g)
            {
                supply.depots[shipment.depot].stock[g] += shipment.cargo[g];
            }
        }
    }
    std::erase_if(supply.pipeline, arrived);
}

void Supply(SimState& state)
{
    std::fill(state.supply.edgeFlowThisHour.begin(), state.supply.edgeFlowThisHour.end(), 0.0);
    ReplenishDepots(state);
    SupplyUnits(state);
}

}
