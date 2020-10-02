cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( WaterCube )

get_filename_component( CINDER_PATH "$ENV{CINDER_PATH}" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

FILE(GLOB SRCFILES src/*)

ci_make_app(
	APP_NAME    "WaterCube"
	SOURCES     ${APP_PATH}/src/WaterCubeApp.cpp
	CINDER_PATH ${CINDER_PATH}
)
