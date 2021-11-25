cmake_minimum_required (VERSION 3.10)



project(TsuTimetableBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_subdirectory(sleepy-discord)

include_directories("rapidjson/include")
include_directories("date/include/date")


add_executable(TsuTimetableBot
	example0.cpp)

target_link_libraries(TsuTimetableBot
	sleepy-discord 
)

target_link_libraries(TsuTimetableBot -lpqxx -lpq)


