cmake_minimum_required (VERSION 2.8)
project (gamuitest)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/gamui")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

file(GLOB MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gamui/*.cpp")

#file(GLOB_RECURSE MY_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/gamui/*.h")
#set (Foo_INCLUDE_DIRS "")
#foreach (_headerFile ${Foo_HEADERS})
#    get_filename_component(_dir ${_headerFile} PATH)
#    list (APPEND Foo_INCLUDE_DIRS ${_dir})
#endforeach()
#list(REMOVE_DUPLICATES Foo_INCLUDE_DIRS)
#include_directories(${Foo_INCLUDE_DIRS})

add_executable(gamuitest ${MY_SOURCES})

include(FindPkgConfig)
find_package(OpenGL REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
find_package(Freetype REQUIRED)

include_directories( ${OPENGL_INCLUDE_DIRS} )
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})

target_link_libraries(gamuitest ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES} )

if(CMAKE_C_COMPILER_ID )
	add_definitions(-Wno-int-to-pointer-cast -Wall)
endif(CMAKE_C_COMPILER_ID )



