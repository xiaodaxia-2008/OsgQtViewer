#include "DemoWindow.h"
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    DemoWindow* win = new DemoWindow;
    win->show();
    app.exec();
    return 0;
}