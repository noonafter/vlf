# CMake settings
cmake_minimum_required(VERSION 3.12.0)

# Project name
project(vlf LANGUAGES CXX C DESCRIPTION "vlf measurer")

# Compile Options
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set MOC RCC UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

message("**************************************************")
message("Check CMake dir:")
message(STATUS "CMAKE version is " ${CMAKE_VERSION})
message(STATUS "CMAKE_BINARY_DIR:" ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_SOURCE_DIR:" ${CMAKE_SOURCE_DIR})
message(STATUS "PROJECT_SOURCE_DIR:" ${PROJECT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_LIST_DIR:" ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR:" ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR:" ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY:" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY:" ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# Find packages
message("**************************************************")
message("Check Qt dir:")

IF(UNIX AND NOT APPLE)
	set(QT_PATH /opt/Qt5.14.2/5.14.2/gcc_64)
ELSEIF(WIN32)
	set(QT_PATH D:/Qt/Qt5.14.2/5.14.2/mingw73_64)
ENDIF()
set(FFTW3_PATH ${CMAKE_SOURCE_DIR}/src/fftw-3.3.5-dll64)
list(APPEND CMAKE_PREFIX_PATH ${QT_PATH} ${FFTW3_PATH})
message(STATUS "Set CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(QT_VERSION_BASE "Qt5")
find_package(${QT_VERSION_BASE} COMPONENTS REQUIRED
	Core
	Gui
	Widgets
	Network
	Multimedia
	Xml
	Svg
	Sql
	Charts
	SerialPort
	LinguistTools
	PrintSupport
	)

message(STATUS "Found Qt version: ${Qt5Core_VERSION_STRING}")



message("**************************************************")
message("Add or Link directories")
##--------------------- Include directories --------------------------------##

##--------------------- Source files ---------------------------------------##
add_subdirectory(src)




