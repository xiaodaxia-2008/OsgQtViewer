#include "DemoWindow.h"
#include <QStatusBar>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <osgManipulator/RotateCylinderDragger>
#include <osgDB/ReadFile>
#include <GizmoDrawable.h>
#include <TouchballManipulator.h>

#include <QApplication>

osg::ref_ptr<osg::Node> createScene()
{
    osg::ref_ptr<osg::MatrixTransform> scene = new osg::MatrixTransform;
    osg::Cylinder *cylinder =
        new osg::Cylinder(osg::Vec3(0.f, 0.f, 0.f), 0.25f, 0.5f);
    osg::ShapeDrawable *sd = new osg::ShapeDrawable(cylinder);
    sd->setColor(osg::Vec4(0.8f, 0.5f, 0.2f, 1.f));
    osg::StateSet *stateSet = sd->getOrCreateStateSet();
    osg::Material *material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttributeAndModes(material);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    scene->addChild(sd);

    osg::ref_ptr<GizmoDrawable> gizmo = new GizmoDrawable;
    gizmo->setTransform(scene.get());
    gizmo->setGizmoMode(GizmoDrawable::MOVE_GIZMO);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(gizmo.get());
    geode->setCullingActive(false); // allow gizmo to always display
    geode->getOrCreateStateSet()->setRenderingHint(
        osg::StateSet::TRANSPARENT_BIN); // always show at last

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());
    root->addChild(geode.get());
    // Create a simple scene with a pretty cylinder
    // osg::Transform *root = new osg::Transform;

    // root->addDrawable(sd);


    // GizmoDrawable *gizmo = new GizmoDrawable();
    // gizmo->setGizmoMode(GizmoDrawable::Mode::MOVE_GIZMO);
    // gizmo->setDisplayScale(4);
    // root->addDrawable(gizmo);
    return root;
}

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent), widget{new osgQt::Widget(this)}
{
    statusBar()->showMessage("Ready", 10000);
    // There is only a single widget to show - the OSG widget
    setCentralWidget(widget);
    widget->getViewer()->setCameraManipulator(new TouchballManipulator());
    widget->setSceneData(createScene());

    // By default the rendered OSG view will only be updated when the widget
    // is resized. If a mouse handler is installed the view will also be
    // updated when a mouse event is recieved
    widget->update();
}
