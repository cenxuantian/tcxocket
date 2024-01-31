cmake_minimum_required(VERSION 3.0.0)
project(tmc)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/tmc)


add_executable(tmc main.cpp)


if(WIN32)
target_link_libraries(tmc ws2_32)
if(MINGW)
target_link_libraries(tmc atomic)
endif()
elseif(UNIX)
target_link_libraries(tmc pthread)
endif()
