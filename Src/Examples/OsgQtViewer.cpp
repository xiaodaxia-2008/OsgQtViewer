#include "Logger.h"

#include <QApplication>
#include <QViewerWidget.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>

using namespace Vis;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QMainWindow win;
    win.setWindowTitle("OsgQtViewer");
    win.setGeometry(50, 50, 800, 600);

    QDockWidget *sidebar = new QDockWidget;
    sidebar->setWidget(new QLabel("hello, robot!"));
    sidebar->setWindowTitle("Sidebar");
    sidebar->setMinimumWidth(200);
    win.addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, sidebar);

    QViewerWidget *viewer_widget = new QViewerWidget;
    std::shared_ptr<Vis::View> v = viewer_widget->GetView();
    v->Axes(); // global axes

    // transparency is not working, why?
    Vis::Handle h = v->Cylinder({0, 0, 0}, 0.5, 1.0, {1, 1, 0, 0.1});

    // test gizmo
    // v->EnableGizmo(h, 4);
    win.setCentralWidget(viewer_widget);

    QMenuBar *menu = win.menuBar();
    menu->addAction("&Open", [&]() {
        auto fpath = QFileDialog::getOpenFileName(&win, "Select an open file",
                                                  "./", "*.stl");
        if (!fpath.isEmpty()) {
            Vis::Handle h = v->Load(fpath.toStdString());
            if (h == Vis::Handle()) {
                win.statusBar()->showMessage(
                    fmt::format("Failed to load file {}", fpath.toStdString())
                        .c_str());
            }
            else {
                win.statusBar()->showMessage(
                    fmt::format("Successfully loaded file {}",
                                fpath.toStdString())
                        .c_str());
            }
        }
    });

    QMenu* view_menu = new QMenu("Views");
    view_menu->addAction(sidebar->toggleViewAction());

    menu->addMenu(view_menu);

    win.statusBar()->showMessage("ready");

    win.show();
    app.exec();
    return 0;
}