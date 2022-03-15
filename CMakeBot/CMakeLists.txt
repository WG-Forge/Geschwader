cmake_minimum_required (VERSION 3.8)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h??)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
 SFML
 GIT_REPOSITORY https://github.com/SFML/SFML.git
 GIT_TAG 2.5.1
)
FetchContent_MakeAvailable(SFML)

add_executable (CMakeBot ${SOURCES} ${HEADERS})

target_include_directories(CMakeBot PRIVATE Dependencies/include/)

find_library(Ws2 Ws2_32.lib REQUIRED)

target_link_libraries(CMakeBot ${Ws2} sfml-graphics sfml-window)

add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Sprites.png
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

set_target_properties(
	CMakeBot PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
