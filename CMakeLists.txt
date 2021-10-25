
cmake_minimum_required (VERSION 3.0)
project(DDOSFizru)

add_subdirectory(sleepy-discord)

include_directories("rapidjson/include")
include_directories("date/include/date")
add_executable(DDOSFizru
	example0.cpp)

ADD_EXECUTABLE(timeCheck timeChecking.cpp)


target_link_libraries(DDOSFizru
	sleepy-discord 
)


target_link_libraries(timeCheck
	sleepy-discord 
)
