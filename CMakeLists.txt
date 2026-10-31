cmake_minimum_required(VERSION 3.21)
project(space_invaders C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.c)

# WIN32: GUI app on Windows (no console window); ignored elsewhere
add_executable(invaders WIN32 ${SOURCES})
target_include_directories(invaders PRIVATE include)

if(TARGET SDL2::SDL2main)
	target_link_libraries(invaders PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(invaders PRIVATE SDL2::SDL2)

if(MSVC)
	target_compile_definitions(invaders PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(invaders PRIVATE /W3)
else()
	target_compile_options(invaders PRIVATE -Wall -Wextra)
endif()

# Put assets (and SDL2.dll on Windows) next to the executable so it runs from the build folder
add_custom_command(TARGET invaders POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:invaders>/assets
)
if(WIN32)
	add_custom_command(TARGET invaders POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:invaders> $<TARGET_FILE_DIR:invaders>
		COMMAND_EXPAND_LISTS
	)
endif()
