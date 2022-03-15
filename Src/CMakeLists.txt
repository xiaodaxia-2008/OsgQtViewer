add_library(OsgQtWidget)
target_sources(
  OsgQtWidget
  PRIVATE osgQtKeyboardMapper.cpp osgQtKeyboardMapper.h osgQtMouseMapper.cpp
          osgQtMouseMapper.h osgQtWidget.cpp osgQtWidget.h)

target_link_libraries(OsgQtWidget PUBLIC Qt5::Widgets)
target_link_libraries(OsgQtWidget PUBLIC ${OPENSCENEGRAPH_LIBRARIES})
# target_link_libraries( OsgQtWidget PUBLIC osg3::osg osg3::osgDB osg3::osgGA #
# osg3::osgUtil osg3::osgViewer # osg3::osgWidget osg3::osgManipulator
# Qt5::Widgets)
target_include_directories(OsgQtWidget PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(OsgQtWidget PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

add_subdirectory(Examples)
