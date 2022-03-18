add_executable(OsgQtViewer OsgQtViewer.cpp)
target_link_libraries(OsgQtViewer PRIVATE QViewerWidget)

install(TARGETS OsgQtViewer RUNTIME DESTINATION bin)
