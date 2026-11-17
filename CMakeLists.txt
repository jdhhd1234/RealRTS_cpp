cmake_minimum_required(VERSION 3.28)
project(RealRTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2.2 결정론 규칙: fast-math 금지. /utf-8은 한국어 주석 때문.
if(MSVC)
    add_compile_options(/W4 /permissive- /fp:precise /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -ffp-contract=off)
endif()

enable_testing()

add_subdirectory(src/mapdata)
add_subdirectory(src/sim)
add_subdirectory(src/app)
add_subdirectory(tests)
