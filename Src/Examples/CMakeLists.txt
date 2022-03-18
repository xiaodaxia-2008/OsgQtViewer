add_executable(BasicViewer BasicViewer.cpp)
target_link_libraries(BasicViewer PRIVATE QViewerWidget)

install(TARGETS BasicViewer RUNTIME DESTINATION bin)
