cmake_minimum_required (VERSION 3.8)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h??)

find_library(Ws2 Ws2_32.lib REQUIRED)

add_executable (CMakeBot ${SOURCES} ${HEADERS})

target_link_libraries(CMakeBot ${Ws2})

add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)

set_target_properties(
	CMakeBot PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
