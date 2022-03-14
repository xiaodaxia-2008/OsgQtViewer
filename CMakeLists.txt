cmake_minimum_required(VERSION 3.13)

project(OsgQtViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(
  Qt5
  COMPONENTS Widgets
  REQUIRED)
set(CMAKE_AUTOMOC ON)

find_package(Fontconfig REQUIRED)

set(OpenSceneGraph_DIR
    $ENV{HOME}/Rvbust/Install/OpenSceneGraph/lib/cmake/OpenSceneGraph)
find_package(
  OpenSceneGraph NO_DEFAULT_PATH
  COMPONENTS osgManipulator
             osgDB
             osgGA
             osgUtil
             osgText
             osgViewer
             osgWidget
  REQUIRED)

add_subdirectory(Src)
