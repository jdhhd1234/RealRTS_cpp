# realrts_sim: 렌더링 없는 시뮬레이션 코어. SDL·ImGui·GDAL을 링크하지 않는다 (3장).
find_package(EnTT CONFIG REQUIRED)

add_library(realrts_sim STATIC
    src/sim_state.cpp
    src/commands.cpp
    src/movement.cpp
    src/pathfinding.cpp
    src/tick_pacer.cpp
)
target_include_directories(realrts_sim PUBLIC include PRIVATE src)
target_link_libraries(realrts_sim PUBLIC realrts_mapdata EnTT::EnTT)
