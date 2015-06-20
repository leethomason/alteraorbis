cmake_minimum_required (VERSION 2.8)
project (alteraorbis)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/gamui")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

### Package setup. ###
include(FindPkgConfig)
find_package(OpenGL REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)

include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})

### GamuiTest ###

file(GLOB GAMUITEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gamui/*.cpp")
add_executable(gamuitest ${GAMUITEST_SOURCES})
target_link_libraries(gamuitest ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} )
target_link_libraries(gamuitest "${CMAKE_CURRENT_SOURCE_DIR}/freetype/libfreetype.a")

### Builder ###

file(GLOB BUILDER_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/builder/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/grinliz/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2/tinyxml2.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/markov/markov.cpp"
							
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/glstreamer.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/squisher.cpp"

							"${CMAKE_CURRENT_SOURCE_DIR}/importers/ac3d.c"
							"${CMAKE_CURRENT_SOURCE_DIR}/importers/import.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/importers/off.cpp"

							"${CMAKE_CURRENT_SOURCE_DIR}/FastLZ/*.c"
)

add_executable(builder ${BUILDER_SOURCES})
target_link_libraries(builder ${SDL2_LIBRARIES} )

### GameDB Reader ###

file(GLOB DBREADER_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/gamedbreader/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/grinliz/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/FastLZ/*.c"
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/glstreamer.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/squisher.cpp"
)

add_executable(dbreader ${DBREADER_SOURCES})
#target_link_libraries(builder ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} )

### Altera ###

file(GLOB ALTERA_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/ai/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/audio/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/gamui/gamui.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/gamui/gamuifreetype.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/micropather/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/scenes/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/script/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/Shiny/src/*.c"
							"${CMAKE_CURRENT_SOURCE_DIR}/widget/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/grinliz/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/xegame/*.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2/tinyxml2.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/win32/main.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c"							
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/glstreamer.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/xarchive/squisher.cpp"
							"${CMAKE_CURRENT_SOURCE_DIR}/markov/markov.cpp"							
							"${CMAKE_CURRENT_SOURCE_DIR}/FastLZ/*.c"
)

add_executable(alteraorbis ${ALTERA_SOURCES})
target_link_libraries(alteraorbis ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
target_link_libraries(alteraorbis "${CMAKE_CURRENT_SOURCE_DIR}/freetype/libfreetype.a")

### General Config ###

if(CMAKE_C_COMPILER_ID )
	add_definitions(-Wno-int-to-pointer-cast -Wall -std=c++11)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DUFO_LINUX_SDL -DGRINLIZ_PROFILE")
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