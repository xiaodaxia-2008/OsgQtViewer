/*
MIT License

Copyright (c) 2020 Sergey Tomilin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "osgQtWidget.h"

#include "osgQtMouseMapper.h"
#include "osgQtKeyboardMapper.h"

#include <osgGA/TrackballManipulator>

using namespace osgQt;

Widget::Widget(osgViewer::Viewer *viewer, osg::Camera *camera, QWidget *parent)
    : QOpenGLWidget(parent), viewer(viewer),
      graphicsWindow(
          viewer->setUpViewerAsEmbeddedInWindow(x(), y(), width(), height()))

{
    setMainCamera(camera);
    // set default manipulator
    osgGA::TrackballManipulator *manipulator = new osgGA::TrackballManipulator;
    viewer->setCameraManipulator(manipulator);
    viewer->getCameraManipulator()->setHomePosition({-2, 0, 1}, {0, 0, 0},
                                                    {0, 0, 1});
    camera->setClearColor(osg::Vec4(1.f, 1.f, 1.f, 1.f));
    // Calculate the projection aspect ration based on the current widget
    // dimentions
    double aspectRatio = this->width();
    aspectRatio /= this->height();
    camera->setProjectionMatrixAsPerspective(30, aspectRatio, 1, 1000);

    setFocusPolicy(Qt::StrongFocus);
    mouse_mapper = new MouseMapper(this);
    keyboard_mapper = new KeyboardMapper(this);
}

void Widget::setMainCamera(osg::Camera *camera)
{
    if (camera != nullptr) {
        viewer->setCamera(camera);
        camera->setGraphicsContext(graphicsWindow.get());
    }
}

void Widget::resizeGL(int w, int h)
{
    // A sanity check
    if (viewer->getCamera() == nullptr) return;

    const int _x = x();
    const int _y = y();

    graphicsWindow->getEventQueue()->windowResize(_x, _y, w, h);
    graphicsWindow->resized(_x, _y, w, h);
    viewer->getCamera()->setViewport(0, 0, w, h);
}

void Widget::paintGL() { viewer->frame(); }
