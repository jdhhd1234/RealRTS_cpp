#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace mapdata
{

// 강한 타입 인덱스 (2.1). 정수와 섞이지 않게 한다.
enum class NodeId : std::uint32_t {};
enum class EdgeId : std::uint32_t {};
enum class RegionId : std::uint32_t {};

template <typename Id>
constexpr std::size_t ToIndex(Id id)
{
    return static_cast<std::size_t>(id);
}

// 진영 번호. gpkg의 side 속성 그대로 (5.4). 0 = Blue, 1 = Red. 양측만 있다 (11장 A5).
using Side = std::uint8_t;
inline constexpr Side kSideCount = 2;

// 아래 enum은 저장 파일에 숫자로 들어가므로 순서를 바꾸지 않는다.
// Terrain 순서는 겹칠 때 우선순위이기도 하다 (5.4): plains < forest < marsh < urban < mountain < water
enum class Terrain : std::uint8_t { Plains, Forest, Marsh, Urban, Mountain, Water };
enum class NodeKind : std::uint8_t { Junction, Station, Port };
enum class EdgeKind : std::uint8_t { Rail, Road, River };

// 게임 좌표 (m). 원점 = 지도 좌하단, x 동쪽+, y 북쪽+ (5.1).
struct Pos
{
    float x = 0.0f;
    float y = 0.0f;
};

// 1 km 정사각 셀 격자 (11장 C2). 행 우선, y = 0이 남쪽 끝 행.
struct CellGrid
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    float cellSize = 1000.0f; // m
    std::vector<Terrain> terrain;
    std::vector<RegionId> region;
};

constexpr std::size_t CellIndex(const CellGrid& grid, std::uint32_t x, std::uint32_t y)
{
    return std::size_t{y} * grid.width + x;
}

// 일 생산량 (t/일).
struct Production
{
    float fuel = 0.0f;
    float ammo = 0.0f;
    float food = 0.0f;
    float equipment = 0.0f;
};

struct Region
{
    std::string name;
    Production production;
};

struct Node
{
    std::string name;
    NodeKind kind = NodeKind::Junction;
    Pos pos;
};

struct Edge
{
    EdgeKind kind = EdgeKind::Rail;
    NodeId from{};
    NodeId to{};
    float capacity = 0.0f; // t/일
    float length = 0.0f;   // m
    std::uint8_t gauge = 0;
};

// 바뀌지 않는 지도: 지형, 행정 구역, 수송망.
struct Map
{
    CellGrid grid;
    std::vector<Region> regions;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
};

struct UnitSpawn
{
    std::string name;
    Side side = 0;
    Pos pos;
};

// 노드에 붙는 보급소 또는 보급 기지.
struct Facility
{
    NodeId node{};
    Side side = 0;
};

// 시작 상황. 에디터(14장)는 이것만 바꾼다.
struct ScenarioData
{
    std::vector<Side> legal;   // 지역별 법적 소유, RegionId 인덱스
    std::vector<Side> control; // 지역별 시작 군사 통제, RegionId 인덱스
    std::vector<UnitSpawn> units;
    std::vector<Facility> depots;
    std::vector<Facility> bases;
};

struct Baked
{
    Map map;
    ScenarioData scenario;
};

// 지도 읽기·검증 실패. 메시지에 원인이 들어간다 (2.1).
class LoadError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// 크기·인덱스·범위가 맞지 않으면 LoadError를 던진다.
void Validate(const Baked& baked);

}
