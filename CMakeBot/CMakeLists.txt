cmake_minimum_required (VERSION 3.8)


set(SOURCES
		CMakeBot.cpp
		ClientWG.cpp
		JSONParser.cpp
)

set(HEADERS
		ClientWG.h
		CMakeBot.h
		json.hpp
		JSONParser.h
)

find_library(Ws2 Ws2_32.lib REQUIRED)

add_executable (CMakeBot ${SOURCES} ${HEADERS})

target_link_libraries(CMakeBot ${Ws2})

set_target_properties(
	CMakeBot PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
