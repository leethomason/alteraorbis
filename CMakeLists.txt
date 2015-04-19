cmake_minimum_required (VERSION 2.6)
project (gamuitest)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/gamui")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

add_executable(gamuitest "${CMAKE_CURRENT_SOURCE_DIR}/gamui/gamuitest.cpp")

find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} )

if(CMAKE_C_COMPILER_ID )
	add_definitions(-Wno-int-to-pointer-cast)
endif(CMAKE_C_COMPILER_ID )

target_link_libraries(gamuitest ${OPENGL_LIBRARIES} )


