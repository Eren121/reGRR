cmake_minimum_required(VERSION 3.6)
project(regrr)

# Library
add_library(regrr src/regrr.cpp)
target_include_directories(regrr PUBLIC include)

find_package(OpenCV REQUIRED)
target_link_libraries(regrr PUBLIC ${OpenCV_LIBS})
target_include_directories(regrr PUBLIC ${OpenCV_INCLUDE_DIRS})

# Test
add_executable(regrr-test main.cpp)
target_link_libraries(regrr-test PRIVATE regrr)

set(ENABLE_REGRR CACHE BOOL "Whether to save files for regression testing")
if(ENABLE_REGRR)
    target_compile_definitions(regrr PUBLIC ENABLE_REGRR=1)
endif()