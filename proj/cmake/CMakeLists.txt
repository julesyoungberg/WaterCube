cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE ON)

project(WaterCube)

get_filename_component(CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../crtv/cinder" ABSOLUTE) # "$ENV{CINDER_PATH}" ABSOLUTE)
get_filename_component(APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE)

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

# FILE(GLOB_RECURSE SRCFILES "${APP_PATH}src/**/*.cpp")
# source_group(SOURCES FILES ${SRCFILES})

list( APPEND SOURCES
	${APP_PATH}/src/core/Container.cpp
	${APP_PATH}/src/core/Fluid.cpp
	${APP_PATH}/src/core/Particle.cpp
	${APP_PATH}/src/core/Scene.cpp
	${APP_PATH}/src/WaterCubeApp.cpp
  )

list( APPEND INCLUDES
    ${APP_PATH}/include/
    ${APP_PATH}/src/core/
    )

ci_make_app(
	APP_NAME    "WaterCube"
	SOURCES     ${SOURCES}
	INCLUDES	${APP_PATH}/include/
	CINDER_PATH ${CINDER_PATH}
)
