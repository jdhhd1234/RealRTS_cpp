find_package(SDL3 CONFIG REQUIRED)

add_executable(realrts src/main.cpp)
target_link_libraries(realrts PRIVATE realrts_sim SDL3::SDL3)

# vcpkg가 SDL3.dll을 실행 파일 옆에 복사한다.
