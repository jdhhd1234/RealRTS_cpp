find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(sim_tests sim_tests.cpp)
target_link_libraries(sim_tests PRIVATE realrts_sim Catch2::Catch2WithMain)
catch_discover_tests(sim_tests)

add_executable(mapdata_tests mapdata_tests.cpp)
target_link_libraries(mapdata_tests PRIVATE realrts_mapdata Catch2::Catch2WithMain)
catch_discover_tests(mapdata_tests)
