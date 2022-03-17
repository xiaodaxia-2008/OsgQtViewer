#include "DemoWindow.h"
#include <QApplication>

// int main(int argc, char **argv)
// {
//     QApplication app(argc, argv);
//     DemoWindow* win = new DemoWindow;
//     win->show();
//     app.exec();
//     return 0;
// }

#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <osgManipulator/RotateCylinderDragger>
#include <osgDB/ReadFile>
#include <GizmoDrawable.h>
int main(int argc, char **argv)
{
    osg::setNotifyLevel(osg::FATAL);
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

    osgViewer::Viewer viewer;
    viewer.setSceneData(root.get());
    viewer.setCameraManipulator(new MyTrackballManipulator);
    viewer.addEventHandler(new osgGA::StateSetManipulator(
        viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.realize();

    osgViewer::GraphicsWindow *gw = dynamic_cast<osgViewer::GraphicsWindow *>(
        viewer.getCamera()->getGraphicsContext());
    if (gw) {
        // Send window size for libGizmo to initialize
        int x, y, w, h;
        gw->getWindowRectangle(x, y, w, h);
        viewer.getEventQueue()->windowResize(x, y, w, h);
    }
    return viewer.run();
}