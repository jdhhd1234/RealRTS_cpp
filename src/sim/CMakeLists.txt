# realrts_sim: 렌더링 없는 시뮬레이션 코어. SDL·ImGui·GDAL을 링크하지 않는다 (3장).
add_library(realrts_sim STATIC
    src/sim_state.cpp
    src/tick_pacer.cpp
)
target_include_directories(realrts_sim PUBLIC include)
