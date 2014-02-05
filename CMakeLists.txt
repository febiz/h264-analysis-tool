cmake_minimum_required( VERSION 2.8 ) 
project( h264-analysis-tool )

## EDIT HERE ##################################################################
# Point to folder where JM library is installed (MANDATORY)
# Tested with JM 18.6
set( JM_DIR C:/libs/JM)
# In case OpenCV is not found please point CMake variable 
# OpenCV_DIR to your build of OpenCV library
set( OpenCV_DIR C:/libs/opencv2.4.4/build ) 
###############################################################################

set( NAME_SRC
    src/main.cpp
	src/macroblockinfo.cpp
	src/vis.cpp
	src/zlib_buffer.cpp
)
set( NAME_HEADERS       
	include/macroblockinfo.h
	include/vis.h
	include/zlib_buffer.h
)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/)

# Include directories
include_directories ("${JM_DIR}/ldecod/inc")
include_directories ("${JM_DIR}/lcommon/inc")
include_directories ("${PROJECT_SOURCE_DIR}/include")

add_executable( H264AnalysisTool ${NAME_SRC} ${NAME_HEADERS} )

# Link libraries
# JM
add_subdirectory (JM)
target_link_libraries( H264AnalysisTool JM )
# OpenCV
find_package( OpenCV REQUIRED )
if (OpenCV_FOUND)
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_link_libraries( H264AnalysisTool ${OpenCV_LIBS} )
endif( OpenCV_FOUND )
# zlib
find_package( ZLIB REQUIRED )
if ( ZLIB_FOUND )
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    target_link_libraries( H264AnalysisTool ${ZLIB_LIBRARIES} )
endif( ZLIB_FOUND )
# winsock
if(WIN32)
  target_link_libraries(H264AnalysisTool wsock32 ws2_32)
endif()

