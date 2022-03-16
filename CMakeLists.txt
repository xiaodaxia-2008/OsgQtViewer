cmake_minimum_required(VERSION 3.13)

# set(CMAKE_INSTALL_PREFIX
#     "/usr/local"
#     CACHE PATH "install prefix")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(OsgQtViewer)
find_package(
  Qt5
  COMPONENTS Widgets
  REQUIRED)
set(CMAKE_AUTOMOC ON)

# set(OpenSceneGraph_DIR
# $ENV{HOME}/Rvbust/Install/OpenSceneGraph/lib/cmake/OpenSceneGraph)
# find_package( OpenSceneGraph NO_DEFAULT_PATH COMPONENTS osgManipulator osgDB
# osgGA osgUtil osgText osgViewer osgWidget REQUIRED)
find_package(
  OpenSceneGraph
  COMPONENTS osgManipulator
             osgDB
             osgGA
             osgUtil
             osgText
             osgViewer
             osgWidget
  REQUIRED)
add_subdirectory(Src)

set(CPACK_PACKAGE_CONTACT "www.rvbust.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "www.rvbust.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "qt5-default, qttools5-dev, qtdeclarative5-dev, libopenscenegraph-3.4-dev")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
include(CPack)
