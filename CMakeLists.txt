cmake_minimum_required (VERSION 2.8)
project (alteraorbis)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/gamui")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

### Package setup. ###
include(FindPkgConfig)
find_package(OpenGL REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
find_package(Freetype REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})

### GamuiTest ###

file(GLOB GAMUITEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gamui/*.cpp")
add_executable(gamuitest ${GAMUITEST_SOURCES})
target_link_libraries(gamuitest ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES} )

### Builder ###

file(GLOB BUILDER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/builder/*.cpp")
add_executable(builder ${BUILDER_SOURCES})
target_link_libraries(builder ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES} )

### General Config ###

if(CMAKE_C_COMPILER_ID )
	add_definitions(-Wno-int-to-pointer-cast -Wall -std=c++11)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
endif(CMAKE_C_COMPILER_ID )


### reference:
#file(GLOB_RECURSE MY_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/gamui/*.h")
#set (Foo_INCLUDE_DIRS "")
#foreach (_headerFile ${Foo_HEADERS})
#    get_filename_component(_dir ${_headerFile} PATH)
#    list (APPEND Foo_INCLUDE_DIRS ${_dir})
#endforeach()
#list(REMOVE_DUPLICATES Foo_INCLUDE_DIRS)
#include_directories(${Foo_INCLUDE_DIRS})