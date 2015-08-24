project (nabla)
cmake_minimum_required (VERSION 2.8)

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set (PACKAGE "nabla")
set (PACKAGE_NAME "nabla")
set (PACKAGE_STRING "Nabla")
set (PACKAGE_URL "http://kiidax.wordpress.com/")
set (PACKAGE_BUGREPORT "katsuya.iida@gmail.com")
set (PACKAGE_VERSION "0.1.0")
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

INCLUDE(CheckIncludeFile)

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake_config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

IF(WIN32)
	SET(MSYS_DIR "C:/MinGW/msys/1.0")
	set(FLEX_EXECUTABLE ${MSYS_DIR}/bin/flex)
	set(BISON_EXECUTABLE ${MSYS_DIR}/bin/bison)
	set(SHELL_EXECUTABLE ${MSYS_DIR}/bin/sh)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /nologo /W3 /Gy /EHsc")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /O2")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Z7 /Od")
	add_definitions(-DHAVE_CONFIG_H -DWIN32 -DGC_NOT_DLL -DPCRE_STATIC)
    SET(GC_DEBUG ${PROJECT_SOURCE_DIR}/third_party/lib/Debug/gc.lib)
    SET(GC_RELEASE ${PROJECT_SOURCE_DIR}/third_party/lib/Release/gc.lib)
    SET(PCRE16_DEBUG ${PROJECT_SOURCE_DIR}/third_party/lib/Debug/pcre16d.lib)
    SET(PCRE16_RELEASE ${PROJECT_SOURCE_DIR}/third_party/lib/Release/pcre16.lib)
    SET(GC debug ${GC_DEBUG} optimized ${GC_RELEASE})
    SET(PCRE16 debug ${PCRE16_DEBUG} optimized ${PCRE16_RELEASE})
ENDIF(WIN32)
IF(UNIX)
    FIND_LIBRARY(GC gc)
    FIND_LIBRARY(PCRE16 pcre16)
	set(FLEX_EXECUTABLE flex)
	set(BISON_EXECUTABLE bison)
	set(SHELL_EXECUTABLE sh)
	set(CMAKE_CXX_FLAGS "-std=c++11")
	add_definitions(-DHAVE_CONFIG_H)
ENDIF(UNIX)
	
add_subdirectory(nabla)
