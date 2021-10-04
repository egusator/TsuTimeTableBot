
cmake_minimum_required (VERSION 3.0)
project(DDOSFizru)

add_subdirectory(sleepy-discord)

include_directories("rapidjson/include")
include_directories("date/include/date")
add_executable(DDOSFizru
	example0.cpp
)

target_link_libraries(DDOSFizru
	sleepy-discord 
)


