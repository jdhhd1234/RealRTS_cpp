# realrts_mapdata: 게임 지도 데이터 타입 + 로더 (sim과 app이 공유, 3장).
find_package(nlohmann_json CONFIG REQUIRED)

add_library(realrts_mapdata STATIC
    src/map.cpp
    src/json_io.cpp
    src/test_map.cpp
)
target_include_directories(realrts_mapdata PUBLIC include)
# JSON 라이브러리는 json_io.cpp 안에만 숨긴다.
target_link_libraries(realrts_mapdata PRIVATE nlohmann_json::nlohmann_json)
