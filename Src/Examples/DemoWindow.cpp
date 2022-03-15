#include "DemoWindow.h"
#include <QStatusBar>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <QApplication>

osg::Geode *createScene()
{
    // Create a simple scene with a pretty cylinder
    osg::Geode *root = new osg::Geode;

    osg::Cylinder *cylinder =
        new osg::Cylinder(osg::Vec3(0.f, 0.f, 0.f), 0.25f, 0.5f);
    osg::ShapeDrawable *sd = new osg::ShapeDrawable(cylinder);
    sd->setColor(osg::Vec4(0.8f, 0.5f, 0.2f, 1.f));
    root->addDrawable(sd);

    osg::StateSet *stateSet = sd->getOrCreateStateSet();
    osg::Material *material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttributeAndModes(material);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

    return root;
}

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent), widget{new osgQt::Widget(this)}
{
    statusBar()->showMessage("Ready", 10000);
    // There is only a single widget to show - the OSG widget
    setCentralWidget(widget);

    widget->setSceneData(createScene());

    // By default the rendered OSG view will only be updated when the widget
    // is resized. If a mouse handler is installed the view will also be
    // updated when a mouse event is recieved
    widget->update();
}
