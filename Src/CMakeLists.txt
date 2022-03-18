add_library(QViewerWidget)
target_sources(
  QViewerWidget
  PRIVATE OsgQtKeyboardMapper.cpp
          OsgQtKeyboardMapper.h
          OsgQtMouseMapper.cpp
          OsgQtMouseMapper.h
          QViewerWidget.cpp
          QViewerWidget.h
          TouchballManipulator.cpp
          TouchballManipulator.h
          GizmoDrawable.h
          Vis.h
          Vis.cpp
          Logger.cpp)

target_link_libraries(QViewerWidget PUBLIC Qt5::Widgets)
target_link_libraries(QViewerWidget PUBLIC stdc++fs)
target_link_libraries(QViewerWidget PUBLIC ${OPENSCENEGRAPH_LIBRARIES} libgizmo
                                           OpenGL::GL)
# target_link_libraries( QViewerWidget PUBLIC osg3::osg osg3::osgDB osg3::osgGA
# # osg3::osgUtil osg3::osgViewer # osg3::osgWidget osg3::osgManipulator
# Qt5::Widgets)
target_include_directories(QViewerWidget PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(QViewerWidget PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

add_subdirectory(Examples)
add_subdirectory(libgizmo)
