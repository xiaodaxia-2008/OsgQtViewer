#include "DemoWindow.h"

osg::Geode *createScene()
{
    // Create a simple scene with a pretty cylinder
    osg::Geode *root = new osg::Geode;

    osg::Cylinder *cylinder =
        new osg::Cylinder(osg::Vec3(0.f, 0.f, 0.f), 0.25f, 0.5f);
    osg::ShapeDrawable *sd = new osg::ShapeDrawable(cylinder);
    sd->setColor(osg::Vec4(0.8f, 0.5f, 0.2f, 1.f));
    root->addDrawable(sd);

    osg::StateSet *stateSet = root->getOrCreateStateSet();
    osg::Material *material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttributeAndModes(material);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

    return root;
}

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent), widget{new osgQt::Widget(this)}
{
    // There is only a single widget to show - the OSG widget
    setCentralWidget(widget);

    // For the demo:
    // Setup the camera so it will be pleasant to look into
    // osgQtDemo::setupCamera(widget);

    // For the demo:
    // Setup a camera manipulator.
    // Since the manipulator reacts on mouse events the mouse handler should
    // be installed.
    // osgQtDemo::setupCameraManipulator(widget);

    // Setup a mouse mapper that translates Qt mouse events to OSG mouse
    // events. This handler is a simple event translator and can be used
    // whenever the OSG view 	should react to mouse events.
    new osgQt::MouseMapper(widget);

    // Setup a keyboard mapper that translates Qt keyboard events to OSG
    // keyboard events. This handler is a simple event translator and can be
    // used whenever the OSG view 	should react to keyboard events.
    new osgQt::KeyboardMapper(widget);

    // Enable the widget to receive key events
    widget->setFocusPolicy(Qt::StrongFocus);

    // Set the scene the main widget should display.
    // This should be after the camera setup - otherwise the camera
    // manipulator 	will reset.
    widget->setSceneData(createScene());

    // By default the rendered OSG view will only be updated when the widget
    // is resized. If a mouse handler is installed the view will also be
    // updated when a mouse event is recieved
}
