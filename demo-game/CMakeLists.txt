cmake_minimum_required(VERSION 3.10)

project(simple-game)

find_package(Curses REQUIRED)
file (GLOB SOURCES *.cpp)
add_executable(simple-game ${SOURCES})
target_link_libraries(simple-game ${CURSES_LIBRARY})

# Enable all warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()
