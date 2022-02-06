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

add_executable (CMakeBot ${SOURCES} ${HEADERS})

set_target_properties(
	CMakeBot PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
