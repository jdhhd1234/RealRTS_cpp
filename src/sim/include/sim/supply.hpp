#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "mapdata/map.hpp"

namespace sim
{

// 물자 3종 (11장 E1).
enum class Good : std::uint8_t { Fuel, Ammo, Food };
inline constexpr std::size_t kGoodCount = 3;

// 물자 묶음 (t), Good 순서로 인덱스. 물자 보존 검사(4.7)가 정확하도록 double.
using Cargo = std::array<double, kGoodCount>;

constexpr std::size_t GoodIndex(Good good)
{
    return static_cast<std::size_t>(good);
}

double Total(const Cargo& cargo);

// ---- 수치 [가정] ----
inline constexpr double kBaseStartStock = 20000.0;  // 품목당 (5.4)
inline constexpr double kDepotTargetStock = 2000.0; // 품목당, 보급소가 채우려는 양
inline constexpr Cargo kUnitTargetStock{600.0, 1000.0, 150.0};
inline constexpr Cargo kIdleUsePerDay{20.0, 0.0, 30.0};
inline constexpr Cargo kMarchUsePerDay{200.0, 0.0, 30.0}; // 탄약 소모는 전투(M5)에서
inline constexpr double kTruckTonsPerHour = 40.0;          // 보급소 -> 사단 1개, 거리 효율 1일 때
inline constexpr float kTruckFullRange = 20000.0f;         // m, 이 안이면 효율 1
inline constexpr float kTruckMaxRange = 100000.0f;         // m, 이 밖이면 고립

// 수송 속도 (m/시간) [가정]: 철도 25 km/h, 도로 20 km/h, 하천 8 km/h.
double TransitMetersPerHour(mapdata::EdgeKind kind);

// 보급소와 사단 사이 트럭 수송 효율 0..1 (4.7 2단계).
double TruckEfficiency(float distanceMeters);

// 보급 기지(국가 재고 진입점) 또는 야전 보급소.
struct SupplyPoint
{
    mapdata::NodeId node{};
    mapdata::Side side = 0;
    Cargo stock{};
};

// 철도·도로 위를 이동 중인 물자 = 파이프라인 재고 (4.7 4단계).
struct Shipment
{
    std::size_t depot = 0; // SupplyState::depots 인덱스
    Cargo cargo{};
    std::uint64_t arriveTick = 0;
};

struct SupplyState
{
    std::vector<SupplyPoint> bases;
    std::vector<SupplyPoint> depots;
    std::vector<Shipment> pipeline;
    std::vector<double> edgeFlowThisHour; // 간선별, 마지막 보급 단계에서 보낸 양 (t)
    Cargo consumed{};                     // 누적 소모량 (보존 검사용)
};

}
