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

#include "QViewerWidget.h"

#include "OsgQtMouseMapper.h"
#include "OsgQtKeyboardMapper.h"
#include "TouchballManipulator.h"

#include <osgGA/TrackballManipulator>

using namespace Vis;

QViewerWidget::QViewerWidget(QWidget *parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
{
    m_view = std::make_shared<Vis::View>();
    m_graphics_window = GetOsgViewer()->setUpViewerAsEmbeddedInWindow(
        x(), y(), width(), height());
    GetOsgViewer()->getCamera()->setGraphicsContext(m_graphics_window.get());

    double aspectRatio = this->width();
    aspectRatio /= this->height();
    GetOsgViewer()->getCamera()->setProjectionMatrixAsPerspective(
        30, aspectRatio, 1, 1000);
    m_mouse_mapper = std::shared_ptr<MouseMapper>(new MouseMapper(this));
    m_keyboard_mapper =
        std::shared_ptr<KeyboardMapper>(new KeyboardMapper(this));
    GetOsgViewer()->home();
    setFocusPolicy(Qt::StrongFocus);
}

void QViewerWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();
    glEnable(GL_DEPTH_TEST);
}

void QViewerWidget::resizeGL(int w, int h)
{
    // A sanity check
    if (GetOsgViewer()->getCamera() == nullptr) return;

    const int _x = x();
    const int _y = y();

    m_graphics_window->getEventQueue()->windowResize(_x, _y, w, h);
    m_graphics_window->resized(_x, _y, w, h);
    GetOsgViewer()->getCamera()->setViewport(0, 0, w, h);
}

void QViewerWidget::paintGL() { GetOsgViewer()->frame(); }

osgViewer::Viewer *QViewerWidget::GetOsgViewer()
{
    return m_view->GetOsgViewer();
}

std::shared_ptr<Vis::View> QViewerWidget::GetView() { return m_view; }