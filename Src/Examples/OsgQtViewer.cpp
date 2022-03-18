#include <QApplication>
#include <QViewerWidget.h>

using namespace Vis;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QViewerWidget *viewer_widget = new QViewerWidget;
    viewer_widget->GetView()->Axes();
    auto h =
        viewer_widget->GetView()->Cylinder({0, 0, 0}, 0.5, 1.0, {1, 1, 0, 0.1});
    viewer_widget->GetView()->EnableGizmo(h, 4);
    viewer_widget->show();
    app.exec();
    return 0;
}