// Copyright (c) RVBUST, Inc - All rights reserved.

#include "Vis.h"

#include "Logger.h"
#include "GizmoDrawable.h"

#include <unordered_map>
#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Material>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>
#include <osg/Point>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <osgFX/Outline>

#include <atomic>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;
using namespace Vis;

static inline bool Vis3d__HasNode(const std::shared_ptr<Vis3d> vis3d,
                                  const Handle &nh)
{
    return vis3d->node_map.find(nh) != vis3d->node_map.end();
}

static inline bool Vis3d__HasOutline(const std::shared_ptr<Vis3d> vis3d,
                                     const Handle &vh)
{
    return vis3d->outlinemap.find(vh) != vis3d->outlinemap.end();
}

void RemoveOutline(const std::shared_ptr<Vis3d> vis3d, const Handle h)
{
    auto node = vis3d->node_map[h];
    auto outline_tmp = vis3d->outlinemap[h];
    vis3d->outlinemap.erase(h);
    outline_tmp->removeChild(node);
    if (outline_tmp->getNumChildren() == 0) {
        vis3d->scene_root->removeChild(outline_tmp);
    }
}

static std::atomic<uint64_t> sg_uid{0};

static inline uint64_t NextHandleID()
{
    return ++sg_uid; // valid from one
}

static inline uint64_t NextObjectID()
{
    return ++sg_uid; // valid from one
}

View::View(osgViewer::Viewer *viewer)
{
    m_vis3d = std::make_shared<Vis3d>();
    m_vis3d->node_switch = new osg::Switch;
    m_vis3d->scene_root = new osg::Group;
    if (viewer) {
        m_vis3d->osgviewer = viewer;
    }
    else {
        m_vis3d->osgviewer = new osgViewer::Viewer;
    }
    m_vis3d->osgviewer->setSceneData(m_vis3d->scene_root);
    m_vis3d->scene_root->addChild(m_vis3d->node_switch);
}

bool View::SetHomePose(const std::array<float, 3> &eye,
                       const std::array<float, 3> &point_want_to_look,
                       const std::array<float, 3> &upvector)
{
    m_vis3d->osgviewer->getCameraManipulator()->setHomePosition(
        {eye[0], eye[1], eye[2]},
        {point_want_to_look[0], point_want_to_look[1], point_want_to_look[2]},
        {upvector[0], upvector[1], upvector[2]}, false);
    return true;
}

bool View::GetHomePose(std::array<float, 3> &eye,
                       std::array<float, 3> &point_want_to_look,
                       std::array<float, 3> &upvector)
{
    osg::Vec3d eye_, point_, up_;
    m_vis3d->osgviewer->getCameraManipulator()->getHomePosition(eye_, point_,
                                                                up_);
    for (int i = 0; i < 3; i++) {
        eye[i] = eye_[i];
        point_want_to_look[i] = point_[i];
        upvector[i] = up_[i];
    }
    return true;
}

bool View::SetCameraPose(const std::array<float, 3> &eye,
                         const std::array<float, 3> &point_want_to_look,
                         const std::array<float, 3> &upvector)
{
    osg::Matrixd vm;
    vm.makeLookAt(
        {eye[0], eye[1], eye[2]},
        {point_want_to_look[0], point_want_to_look[1], point_want_to_look[2]},
        {upvector[0], upvector[1], upvector[2]});
    m_vis3d->osgviewer->getCameraManipulator()->setByInverseMatrix(vm);
    return true;
}

bool View::GetCameraPose(std::array<float, 3> &eye,
                         std::array<float, 3> &point_want_to_look,
                         std::array<float, 3> &upvector, float look_distance)
{
    osg::Vec3d eye_, point_, up_;
    osg::Matrixd vm =
        m_vis3d->osgviewer->getCameraManipulator()->getInverseMatrix();
    vm.getLookAt(eye_, point_, up_, look_distance);
    for (int i = 0; i < 3; i++) {
        eye[i] = eye_[i];
        point_want_to_look[i] = point_[i];
        upvector[i] = up_[i];
    }

    return true;
}

std::pair<float, float> View::GetViewSize()
{
    auto vp = m_vis3d->osgviewer->getCamera()->getViewport();
    return std::make_pair(vp->width(), vp->height());
}

bool View::Clear()
{
    const int num = m_vis3d->node_switch->getNumChildren();
    // remove outline
    m_vis3d->scene_root->removeChild(1,
                                     m_vis3d->scene_root->getNumChildren() - 1);
    m_vis3d->node_switch->removeChildren(0, num);
    m_vis3d->outlinemap.clear();
    m_vis3d->node_map.clear();
    return true;
}

bool View::Delete(const Handle &nh)
{
    const Handle who = nh;
    // LOG_DEBUG("Delete node: type: {0}, uid: {1}.", who.type, who.uid);
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }
    if (Vis3d__HasOutline(m_vis3d, who)) {
        RemoveOutline(m_vis3d, who);
    }

    m_vis3d->node_switch->removeChild(m_vis3d->node_map[who]);
    m_vis3d->node_map.erase(who);
    return true;
}

bool View::IsAlive(const Handle &nh) const
{
    return Vis3d__HasNode(m_vis3d, nh);
}

bool View::Delete(const std::vector<Handle> &handles)
{
    for (const auto h : handles) {
        if (!Delete(h)) {
            LOG_ERROR("Delete handle : ({0}, {1}) failed!", h.type, h.uid);
            return false;
        }
    }
    return true;
}

bool View::Home()
{
    m_vis3d->osgviewer->home();
    return true;
}

bool View::Show(const Handle &nh)
{
    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }
    m_vis3d->node_switch->setChildValue(m_vis3d->node_map[who], true);
    return true;
}

bool View::Hide(const Handle &nh)
{
    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }
    m_vis3d->node_switch->setChildValue(m_vis3d->node_map[who], false);
    return true;
}

bool View::Chain(const std::vector<Handle> &links)
{
    if (links.size() < 1) {
        LOG_ERROR("size of links should be more than 1: {0}", links.size());
        return false;
    }

    { // check if we have invalid or duplicated handles
        std::unordered_set<Handle, HandleHasher> s;
        for (const auto h : links) {
            if (!Vis3d__HasNode(m_vis3d, h)) {
                LOG_ERROR("Can not find node: type: {0}, uid: {1}.", h.type,
                          h.uid);
                return false;
            }

            if (s.find(h) != s.end()) {
                LOG_ERROR("Same links passed into Chain, which is ilegal!");
                return false;
            }
            else {
                s.insert(h);
            }
        }
    }

    for (int i = 1; i < (int)links.size(); ++i) {
        const auto mt = m_vis3d->node_map[links[i]];
        // Unchain from its parents
        const int np = mt->getNumParents();
        for (int j = 0; j < np; ++j) {
            mt->getParent(j)->asGroup()->removeChild(mt);
        }
        // Chain
        m_vis3d->node_map[links[(size_t)i - 1]]->addChild(mt);
    }
    return true;
}

bool View::Unchain(const std::vector<Handle> &links)
{
    if (links.size() < 1) {
        LOG_ERROR("links.size() should be more than 1: {0}", links.size());
        return false;
    }

    for (const auto &h : links) {
        if (!Vis3d__HasNode(m_vis3d, h)) {
            LOG_ERROR("Can not find node: type: {0}, uid: {1}.", h.type, h.uid);
            return false;
        }
    }

    for (int i = 1; i < (int)links.size(); ++i) {
        auto &prev = m_vis3d->node_map[links[i - 1]];
        auto &curr = m_vis3d->node_map[links[i]];
        if (!prev->removeChild(curr)) {
            LOG_ERROR("No link between links[{0}] and links[{1}].", i - 1, i);
        }
        else {
            // Here we should not lose any nodes, so after unchain, if a node is
            // orphan, we need to give it a parent...
            if (curr->getNumParents() == 0) {
                m_vis3d->node_switch->addChild(curr);
            }
        }
    }
    return true;
}

bool View::SetTransforms(const std::vector<Handle> &hs,
                         const std::vector<std::array<float, 3>> &trans,
                         const std::vector<std::array<float, 4>> &quats)
{
    const int hs_size = hs.size();
    const int trans_size = trans.size();
    const int quats_size = quats.size();

    if (hs_size < 1 || (trans_size != hs_size || quats_size != hs_size)) {
        LOG_ERROR("Invalid parameters: hs.size: {0}, trans.size: {1}, "
                  "quats.size: {2}",
                  hs_size, trans_size, quats_size);
        return false;
    }

    std::vector<osg::Matrixf> transforms(trans_size);
    for (int i = 0; i < trans_size; ++i) {
        transforms[i].setRotate(
            osg::Quat(quats[i][0], quats[i][1], quats[i][2], quats[i][3]));
        transforms[i].setTrans(
            osg::Vec3f(trans[i][0], trans[i][1], trans[i][2]));
    }

    if (hs_size != (int)transforms.size()) {
        LOG_ERROR("handles.size() != transforms.size(). We should've checked "
                  "before, shouldn't reach "
                  "here!");
        return false;
    }

    for (int i = 0; i < hs_size; ++i) {
        const Handle h = hs[i];
        if (!Vis3d__HasNode(m_vis3d, h)) {
            LOG_WARN("Could not find object: type: {0}, uid: {1}", h.type,
                     h.uid);
        }
        else {
            m_vis3d->node_map[h]->setMatrix(transforms[i]);
            if (h == m_vis3d->gizmo.refHandle) {
                for (int i = 0; i < 16; ++i) {
                    m_vis3d->gizmo.matrix[i] = *(transforms[0].ptr() + i);
                }
            }
        }
    }

    return true;
}

bool View::SetTransforms(const Handle &h,
                         const std::vector<std::array<float, 3>> &trans,
                         const std::vector<std::array<float, 4>> &quats)
{
    const int trans_size = trans.size();
    const int quats_size = quats.size();
    if (trans_size != quats_size) {
        LOG_ERROR("Invalid parameters: trans.size: {0}, quats.size: {1}",
                  trans_size, quats_size);
        return false;
    }

    std::vector<osg::Matrixf> transforms(trans_size);
    int i = 0;
    for (; i < trans_size; ++i) {
        transforms[i].setRotate(
            osg::Quat(quats[i][0], quats[i][1], quats[i][2], quats[i][3]));
        transforms[i].setTrans(
            osg::Vec3f(trans[i][0], trans[i][1], trans[i][2]));
    }
    // we need to check the base link has the right structure to perform this
    // operations:
    // 1. base link has enough children and each parenth has only one kid
    // following the line.
    if (!Vis3d__HasNode(m_vis3d, h)) {
        LOG_ERROR("Could not find object: type: {0}, uid: {1}", h.type, h.uid);
        return false;
    }

    // For the following, check:
    auto mt = m_vis3d->node_map[h];
    for (i = 1; i < trans_size; ++i) {
        // each MatrixTransform will at least have its own child.
        // It another object is attached to it. It will have a child with type
        // osg::MatrixTransform, thus we the number of children should be 2
        if (mt->getNumChildren() == 2) {
            mt = dynamic_cast<osg::MatrixTransform *>(mt->getChild(1));
            if (!mt) {
                LOG_ERROR("Invalid node!!!");
                return false;
            }
        }
        else {
            break;
        }
    }

    if (i < trans_size) {
        LOG_ERROR("Object doesn't have enough single descendant(s)!");
        return false;
    }

    // Set the transform
    mt = m_vis3d->node_map[h];
    for (i = 0; i < trans_size - 1; ++i) {
        mt->setMatrix(transforms[i]);
        mt = dynamic_cast<osg::MatrixTransform *>(mt->getChild(1));
        if (!mt) {
            LOG_ERROR("Invalid node!!!");
            return false;
        }
    }
    mt->setMatrix(transforms[i]);
    if (h == m_vis3d->gizmo.refHandle) {
        for (int i = 0; i < 16; ++i) {
            m_vis3d->gizmo.matrix[i] = *(transforms[0].ptr() + i);
        }
    }

    return true;
}

// TODO(Hui): In OSG, different objects has different ways of setting
// transparency, which needs some time to implement and test. For now,
// we only support Model, Box, Sphere, Cylinder, Cone here.
// https://blog.csdn.net/wang15061955806/article/details/49466337
bool View::SetTransparency(const Handle &nh, float inv_alpha)
{
    if (nh.type != ViewObjectType_Model && nh.type != ViewObjectType_Box
        && nh.type != ViewObjectType_Sphere
        && nh.type != ViewObjectType_Cylinder
        && nh.type != ViewObjectType_Cone) {
        LOG_ERROR(
            "Currently only Model, Box, Sphere, Cylinder type is supported.");
        return false;
    }

    if (inv_alpha < 0.f || inv_alpha > 1.f) {
        LOG_ERROR("Invalid alpha value: {0}, should be in range [0, 1]",
                  inv_alpha);
        return false;
    }

    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }

    auto mt = m_vis3d->node_map[who];
    const float alpha = 1.f - inv_alpha;
    if (who.type == ViewObjectType_Model) {
        osg::Node *node = mt->getChild(0);
        if (node == nullptr) {
            LOG_ERROR(
                "No child for the node, which should be impossible! ({0}, {1})",
                who.type, who.uid);
            return false;
        }
        auto ss = node->getOrCreateStateSet();
        ss->setMode(GL_BLEND,
                    osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
        osg::Material *material =
            (osg::Material *)ss->getAttribute(osg::StateAttribute::MATERIAL);
        if (!material) {
            material = new osg::Material;
        }

        material->setAlpha(osg::Material::FRONT_AND_BACK, alpha);

        if (alpha >= 1.0f) {
            ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            ss->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE
                                                   | osg::StateAttribute::OFF);
        }
        else {
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            ss->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE
                                                   | osg::StateAttribute::ON);
        }
    }
    else {
        /// ShapeDrawables
        osg::Geode *gnode = static_cast<osg::Geode *>(mt->getChild(0));
        if (gnode == nullptr) {
            LOG_ERROR("No child for the geode, which should be impossible! "
                      "({0}, {1})",
                      who.type, who.uid);
            return false;
        }
        osg::ShapeDrawable *sd =
            static_cast<osg::ShapeDrawable *>(gnode->getDrawable(0));
        if (sd == nullptr) {
            LOG_ERROR("No child for the geode, which should be impossible! "
                      "({0}, {1})",
                      who.type, who.uid);
            return false;
        }
        // TODO: we need to check if alpha is in range [0, 1]
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
        osg::Vec4 color = sd->getColor();
        color[3] = alpha;
        sd->setColor(color);
        LOG_DEBUG("SetTransparency: [{0},{1},{2},{3}]", color[0], color[1],
                  color[2], color[3]);
    }
    return true;
}

bool View::ShowOutline(const Handle &nh, bool show, float width,
                       const std::vector<float> &color)
{
    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }
    if (width < 0) {
        LOG_ERROR("Width should be positive");
        return false;
    }
    const int color_size = color.size();
    if (color_size != 3 && color_size != 4) {
        LOG_ERROR("Color should size 3 or 4, color.size() == {0}!", color_size);
        return false;
    }
    osg::Vec4 color_;
    if (color_size == 3) {
        color_ = {color[0], color[1], color[2], 1};
    }
    else {
        color_ = {color[0], color[1], color[2], color[3]};
    }

    auto node = m_vis3d->node_map[who];
    bool outline_shown = Vis3d__HasOutline(m_vis3d, who);

    if (outline_shown) {
        RemoveOutline(m_vis3d, who);
    }
    if (show) {
        osg::ref_ptr<osgFX::Outline> outline = new osgFX::Outline;
        m_vis3d->outlinemap[who] = outline;
        outline->setWidth(width);
        outline->setColor(color_);
        outline->addChild(node);
        m_vis3d->scene_root->addChild(outline);
    }
    return true;
}

bool View::ShowOutline(const std::vector<Handle> &hs, bool show, float width,
                       const std::vector<float> &color)
{
    for (auto h : hs) {
        if (!Vis3d__HasNode(m_vis3d, h)) {
            LOG_ERROR("Can not find node: type: {0}, uid: {1}.", h.type, h.uid);
            return false;
        }
    }

    if (width < 0) {
        LOG_ERROR("Width should be positive");
        return false;
    }

    const int color_size = color.size();
    if (color_size != 3 && color_size != 4) {
        LOG_ERROR("Color should size 3 or 4, color.size() == {0}!", color_size);
        return false;
    }
    osg::Vec4 color_;
    if (color_size == 3) {
        color_ = {color[0], color[1], color[2], 1};
    }
    else {
        color_ = {color[0], color[1], color[2], color[3]};
    }

    if (show) {
        osg::ref_ptr<osgFX::Outline> outline = new osgFX::Outline;
        outline->setColor(color_);
        outline->setWidth(width);

        for (auto h : hs) {
            auto node = m_vis3d->node_map[h];
            if (Vis3d__HasOutline(m_vis3d, h)) {
                RemoveOutline(m_vis3d, h);
            }
            m_vis3d->outlinemap[h] = outline;
            outline->addChild(node);
        }
        m_vis3d->scene_root->addChild(outline);
    }
    else {
        for (auto h : hs) {
            ShowOutline(h, show, width, color);
        }
    }

    return true;
}

bool View::SetColor(const Handle &nh, const std::vector<float> &color,
                    int color_channels)
{
    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }

    const int color_size = color.size();
    if (color_size % 3 != 0 && color_size % 4 != 0) {
        LOG_ERROR("Color should size 3 or 4, color.size() == {0}!", color_size);
        return false;
    }

    if (color_channels == 0 && (color_size == 3 || color_size == 4)) {
        color_channels = color_size;
    }

    if (color_channels != 3 && color_channels != 4) {
        LOG_ERROR("Color channels [{}] must be 3 or 4.", color_channels);
        return false;
    }

    if (who.type == ViewObjectType_Model) {
        const osg::ref_ptr<osg::MatrixTransform> mt = m_vis3d->node_map[who];
        osg::Node *node = mt->getChild(0);
        if (node == nullptr) return false;

        auto ss = node->getOrCreateStateSet();
        ss->setMode(GL_BLEND,
                    osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
        osg::Material *material =
            (osg::Material *)ss->getAttribute(osg::StateAttribute::MATERIAL);
        if (!material) {
            material = new osg::Material;
        }

        osg::Vec4d orig_color =
            material->getDiffuse(osg::Material::FRONT_AND_BACK);

        LOG_DEBUG("orig_color: {0}, {1}, {2}, {3}", orig_color[0],
                  orig_color[1], orig_color[2], orig_color[3]);

        for (int i = 0; i < 4; ++i) {
            orig_color[i] = color[i];
        }

        material->setDiffuse(osg::Material::FRONT_AND_BACK, orig_color);

        const float alpha = orig_color[3];

        if (alpha >= 1.0f) {
            ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            ss->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE
                                                   | osg::StateAttribute::OFF);
        }
        else {
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            ss->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE
                                                   | osg::StateAttribute::ON);
        }
    }
    else if (who.type > ViewObjectType_Cylinder
             || who.type < ViewObjectType_Point) {
        LOG_ERROR(u8"not support");
        return false;
    }
    else {
        /// ShapeDrawables
        const osg::ref_ptr<osg::MatrixTransform> mt = m_vis3d->node_map[who];
        osg::Geode *geode = static_cast<osg::Geode *>(mt->getChild(0));
        if (geode == nullptr) {
            LOG_ERROR("No child for the geode, which should be impossible! "
                      "({0}, {1})",
                      who.type, who.uid);
            return false;
        }
        osg::Drawable *drawable = geode->getDrawable(0);
        if (drawable == nullptr) {
            LOG_ERROR("No child for the geode, which should be impossible! "
                      "({0}, {1})",
                      who.type, who.uid);
            return false;
        }
        // TODO: we need to check if alpha is in range [0, 1]
        drawable->getOrCreateStateSet()->setMode(GL_BLEND,
                                                 osg::StateAttribute::ON);
        drawable->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);

        osg::Geometry *geom = drawable->asGeometry();
        const int color_array_size = color_size / color_channels;
        if (geom != nullptr) {
            auto vertices = geom->getVertexArray();
            if (color_array_size != 1
                && color_array_size != (int)vertices->getDataSize()) {
                LOG_ERROR(
                    "color array size [{}] not match vertex array size [{}]",
                    color_array_size, vertices->getDataSize());
                return false;
            }
            osg::ref_ptr<osg::Array> color_array;
            if (color_channels == 3) {
                color_array = new osg::Vec3Array(
                    color_array_size, (const osg::Vec3 *)(color.data()));
            }
            else {
                color_array = new osg::Vec4Array(
                    color_array_size, (const osg::Vec4 *)(color.data()));
            }

            geom->setColorArray(color_array, color_array_size == 1
                                                 ? osg::Array::BIND_OVERALL
                                                 : osg::Array::BIND_PER_VERTEX);
        }
        else {
            osg::ShapeDrawable *sd =
                static_cast<osg::ShapeDrawable *>(drawable);
            if (color_array_size != 1) {
                LOG_ERROR("too much color to set. {}", color_array_size);
                return false;
            }
            osg::Vec4 color_old = sd->getColor();
            for (int i = 0; i < color_channels; i++) {
                color_old[i] = color[i];
            }
            sd->setColor(color_old);
        }
    }
    return true;
}

bool View::GetColor(const Handle &nh, std::vector<float> &color) const
{
    const Handle who = nh;
    if (!Vis3d__HasNode(m_vis3d, who)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", who.type, who.uid);
        return false;
    }

    if (who.type < ViewObjectType_Point || who.type > ViewObjectType_Cylinder) {
        LOG_ERROR(
            "Can not get color from this object type: type: {0}, uid: {1}.",
            who.type, who.uid);
        return false;
    }

    const osg::ref_ptr<osg::MatrixTransform> mt = m_vis3d->node_map[who];
    osg::Geode *gnode = static_cast<osg::Geode *>(mt->getChild(0));
    if (gnode == nullptr) {
        LOG_ERROR(
            "No child for the geode, which should be impossible! ({0}, {1})",
            who.type, who.uid);
        return false;
    }
    osg::Drawable *drawable = gnode->getDrawable(0);
    if (drawable == nullptr) {
        LOG_ERROR(
            "No child for the geode, which should be impossible! ({0}, {1})",
            who.type, who.uid);
        return false;
    }
    // Geometry
    if (drawable->asGeometry()) {
        osg::Geometry *geom = drawable->asGeometry();
        auto color_array = geom->getColorArray();
        auto color_array_size = color_array->getNumElements();
        auto color_channel =
            color_array->getDataType() == osg::Array::Vec3ArrayType ? 3 : 4;
        color.resize(color_array_size * 4);
        if (color_channel == 3) {
            osg::Vec3Array *color3array =
                dynamic_cast<osg::Vec3Array *>(color_array);
            for (unsigned int i = 0; i < color_array_size; i++) {
                color[i * 4 + 0] = (*color3array)[i].x();
                color[i * 4 + 1] = (*color3array)[i].y();
                color[i * 4 + 2] = (*color3array)[i].z();
                color[i * 4 + 3] = 1.0f;
            }
        }
        else {
            osg::Vec4Array *color4array =
                dynamic_cast<osg::Vec4Array *>(color_array);
            for (unsigned int i = 0; i < color_array_size; i++) {
                color[i * 4 + 0] = (*color4array)[i].x();
                color[i * 4 + 1] = (*color4array)[i].y();
                color[i * 4 + 2] = (*color4array)[i].z();
                color[i * 4 + 3] = (*color4array)[i].a();
            }
        }
    }
    else {
        // ShapeDrawables
        color.resize(4);
        osg::ShapeDrawable *sd = static_cast<osg::ShapeDrawable *>(drawable);
        osg::Vec4d orig_color = sd->getColor();
        for (int i = 0; i < 4; ++i) {
            color[i] = orig_color[i];
        }
    }

    return true;
}

bool View::SetPosition(const Handle &nh, const std::array<float, 3> &trans)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }

    osg::Matrixf m = m_vis3d->node_map[nh]->getMatrix();
    m.setTrans(osg::Vec3f(trans[0], trans[1], trans[2]));
    m_vis3d->node_map[nh]->setMatrix(m);
    return true;
}

bool View::SetRotation(const Handle &nh, const std::array<float, 4> &quat)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }

    osg::Matrixf m = m_vis3d->node_map[nh]->getMatrix();
    m.setRotate(osg::Quat(quat[0], quat[1], quat[2], quat[3]));
    m_vis3d->node_map[nh]->setMatrix(m);
    return true;
}

bool View::SetTransform(const Handle &nh, const std::array<float, 3> &trans,
                        const std::array<float, 4> &quat)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }
    osg::Matrixf m;
    m.setRotate(osg::Quat(quat[0], quat[1], quat[2], quat[3]));
    m.setTrans(osg::Vec3f(trans[0], trans[1], trans[2]));
    m_vis3d->node_map[nh]->setMatrix(m);
    if (nh == m_vis3d->gizmo.refHandle) {
        for (int i = 0; i < 16; ++i) {
            m_vis3d->gizmo.matrix[i] = *(m.ptr() + i);
        }
    }
    return true;
}

bool View::GetTransform(const Handle &nh, std::array<float, 3> &pos,
                        std::array<float, 4> &quat)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }

    auto transform = m_vis3d->node_map[nh]->getMatrix();

    const osg::Quat q = transform.getRotate();
    const osg::Vec3f trans = transform.getTrans();
    quat[0] = q.x();
    quat[1] = q.y();
    quat[2] = q.z();
    quat[3] = q.w();
    pos[0] = trans.x();
    pos[1] = trans.y();
    pos[2] = trans.z();
    return true;
}

bool View::GetPosition(const Handle &nh, std::array<float, 3> &pos)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }

    auto transform = m_vis3d->node_map[nh]->getMatrix();
    const osg::Vec3f trans = transform.getTrans();
    pos[0] = trans.x();
    pos[1] = trans.y();
    pos[2] = trans.z();
    return true;
}

bool View::GetRotation(const Handle &nh, std::array<float, 4> &quat)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return false;
    }

    auto transform = m_vis3d->node_map[nh]->getMatrix();

    const osg::Quat q = transform.getRotate();
    quat[0] = q.x();
    quat[1] = q.y();
    quat[2] = q.z();
    quat[3] = q.w();
    return true;
}

Handle View::Clone(const Handle &nh, const osg::Matrix &m)
{
    Handle dst;
    osg::ref_ptr<osg::MatrixTransform> mt =
        dynamic_cast<osg::MatrixTransform *>(
            m_vis3d->node_map[nh]->clone(osg::CopyOp::DEEP_COPY_ALL));
    mt->setMatrix(m);
    dst.type = nh.type;
    dst.uid = NextHandleID();
    mt->setName(std::string("mt") + std::to_string(NextObjectID()));
    m_vis3d->node_switch->addChild(0, mt);
    m_vis3d->node_map.insert({dst, mt});
    return dst;
}

Handle View::Clone(const Handle nh)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return Handle();
    }
    osg::Matrix transform = m_vis3d->node_map[nh]->getMatrix();

    return Clone(nh, transform);
}

Handle View::Clone(const Handle nh, const std::array<float, 3> &pos,
                   const std::array<float, 4> &quat)
{
    if (!Vis3d__HasNode(m_vis3d, nh)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", nh.type, nh.uid);
        return Handle();
    }

    osg::Matrix transform;
    transform.setRotate(osg::Quat(quat[0], quat[1], quat[2], quat[3]));
    transform.setTrans(osg::Vec3f(pos[0], pos[1], pos[2]));
    return Clone(nh, transform);
}

std::vector<Handle> View::Clone(const std::vector<Handle> &handles)
{
    std::vector<Handle> newhs(handles.size());
    int i = 0;
    for (auto &h : handles) {
        newhs[i++] = Clone(h);
    }
    return newhs;
}

std::vector<Handle> View::Clone(const std::vector<Handle> &handles,
                                const std::vector<std::array<float, 3>> &poss,
                                const std::vector<std::array<float, 4>> &quats)
{
    const int hs_size = handles.size();
    const int trans_size = poss.size();
    const int quats_size = quats.size();

    if (hs_size < 1 || (trans_size != hs_size || quats_size != hs_size)) {
        LOG_ERROR("Invalid parameters: hs.size: {0}, trans.size: {1}, "
                  "quats.size: {2}",
                  hs_size, trans_size, quats_size);
        return {};
    }

    std::vector<Handle> newhs(hs_size);
    osg::Matrix transform;
    for (int i = 0; i < hs_size; ++i) {
        transform.setRotate(
            osg::Quat(quats[i][0], quats[i][1], quats[i][2], quats[i][3]));
        transform.setTrans(osg::Vec3f(poss[i][0], poss[i][1], poss[i][2]));
        newhs[i] = Clone(handles[i], transform);
    }

    return newhs;
}

Handle View::Picked()
{
    // Not thread safe...
    return m_vis3d->picked;
}

void View::UnPick() { m_vis3d->picked.Reset(); }

std::array<float, 6> View::PickedPlane() { return m_vis3d->pointnorm; }

Handle View::Load(const std::string &fname, const osg::Matrix &m)
{
    Handle h;
    osg::ref_ptr<osg::Node> model = osgDB::readNodeFile(fname.c_str());
    if (!model) {
        LOG_ERROR("Read model {0} failed!", fname);
        return h;
    }

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;

    /// STL model doesn't have color information
    if (fs::path(fname).extension() == ".stl") {
        LOG_DEBUG("Adding a materal for STL file.");
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK,
                             osg::Vec4(0.5, 0.5, 0.5, 1.0));
        model->getOrCreateStateSet()->setAttributeAndModes(
            material, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }

    mt->addChild(model);

    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));
    model->setName(std::to_string(NextObjectID()));

    mt->setMatrix(m);

    h.type = ViewObjectType_Model;
    h.uid = NextHandleID();
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Load(const std::string &fname)
{
    osg::Matrix transform;
    return Load(fname, transform);
}

Handle View::Load(const std::string &fname, const std::array<float, 3> &pos,
                  const std::array<float, 4> &quat)
{
    osg::Matrix transform;
    transform.setRotate(osg::Quat(quat[0], quat[1], quat[2], quat[3]));
    transform.setTrans(osg::Vec3f(pos[0], pos[1], pos[2]));
    return Load(fname, transform);
}

std::vector<Handle> View::Load(const std::vector<std::string> &fnames)
{
    std::vector<Handle> hs(fnames.size());
    int i = 0;
    osg::Matrix transform;
    for (auto &name : fnames) {
        hs[i++] = Load(name, transform);
    }
    return hs;
}

std::vector<Handle> View::Load(const std::vector<std::string> &fnames,
                               const std::vector<std::array<float, 3>> &poss,
                               const std::vector<std::array<float, 4>> &quats)
{
    std::vector<Handle> hs(fnames.size());
    int i = 0;
    osg::Matrix transform;
    for (auto &name : fnames) {
        transform.setRotate(
            osg::Quat(quats[i][0], quats[i][1], quats[i][2], quats[i][3]));
        transform.setTrans(osg::Vec3f(poss[i][0], poss[i][1], poss[i][2]));
        hs[i++] = Load(name, transform);
    }

    return hs;
}

Handle View::Axes(const osg::Matrix &m, float axis_len, float axis_size)
{
    Handle h;
    if (axis_len <= 0 || axis_size <= 0) {
        LOG_ERROR("Invalid axis len or size: {0}, {1}", axis_len, axis_size);
        return h;
    }

    osg::ref_ptr<osg::Geometry> geo = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> v = new osg::Vec3Array(6);
    (*v)[0] = osg::Vec3f(0.0f, 0.0f, 0.0f);
    (*v)[1] = osg::Vec3f(axis_len, 0.0f, 0.0f);
    (*v)[2] = osg::Vec3f(0.0f, 0.0f, 0.0f);
    (*v)[3] = osg::Vec3f(0.0f, axis_len, 0.0f);
    (*v)[4] = osg::Vec3f(0.0f, 0.0f, 0.0f);
    (*v)[5] = osg::Vec3f(0.0f, 0.0f, axis_len);

    osg::ref_ptr<osg::Vec4Array> c = new osg::Vec4Array(6);
    (*c)[0] = osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
    (*c)[1] = osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f); // x red
    (*c)[2] = osg::Vec4f(0.0f, 1.0f, 0.0f, 1.0f);
    (*c)[3] = osg::Vec4f(0.0f, 1.0f, 0.0f, 1.0f); // y green
    (*c)[4] = osg::Vec4f(0.0f, 0.0f, 1.0f, 1.0f);
    (*c)[5] = osg::Vec4f(0.0f, 0.0f, 1.0f, 1.0f); // z blue

    geo->setVertexArray(v);
    geo->setColorArray(c);
    geo->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geo->addPrimitiveSet(
        new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2)); // X
    geo->addPrimitiveSet(
        new osg::DrawArrays(osg::PrimitiveSet::LINES, 2, 2)); // Y
    geo->addPrimitiveSet(
        new osg::DrawArrays(osg::PrimitiveSet::LINES, 4, 2)); // Z
    geo->getOrCreateStateSet()->setAttribute(new osg::LineWidth(axis_size),
                                             osg::StateAttribute::ON);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING,
                                          osg::StateAttribute::OFF);
    geode->addDrawable(geo);

    h.type = ViewObjectType_Axes;
    h.uid = NextHandleID();

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->addChild(geode);

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Axes(const std::array<float, 3> &trans,
                  const std::array<float, 4> &quat, float axis_len,
                  float axis_size)
{
    osg::Matrix transform;
    transform.setRotate(osg::Quat(quat[0], quat[1], quat[2], quat[3]));
    transform.setTrans(osg::Vec3f(trans[0], trans[1], trans[2]));
    return Axes(transform, axis_len, axis_size);
}

Handle View::Axes(const std::array<float, 16> &transform, float axis_len,
                  float axis_size)
{
    osg::Matrix m;
#define AT(i, j) transform[i + j * 4]
    m.set(AT(0, 0), AT(0, 1), AT(0, 2), AT(0, 3), AT(1, 0), AT(1, 1), AT(1, 2),
          AT(1, 3), AT(2, 0), AT(2, 1), AT(2, 2), AT(2, 3), AT(3, 0), AT(3, 1),
          AT(3, 2), AT(3, 3));
#undef AT

    return Axes(m, axis_len, axis_size);
}

Handle View::Point(const std::vector<float> &xyzs, float size,
                   const std::vector<float> &colors)
{
    Handle h;
    const size_t xyzs_size = xyzs.size();
    const size_t numpt = xyzs_size / 3;
    const size_t colors_size = colors.size();

    if (xyzs_size == 0 || xyzs_size % 3 != 0) {
        LOG_WARN("xyzs.size() is wrong! {0}", xyzs_size);
        return h;
    }

    if (size <= 0) {
        LOG_WARN("point size is wrong! {0}", size);
        return h;
    }

    if (colors_size == 0 || (colors_size % 3 != 0 && colors_size % 4 != 0)) {
        LOG_WARN("colors.size is wrong! {0}", colors_size);
        return h;
    }

    size_t color_channels = 0;
    if (colors_size % 3 == 0 && colors_size % 4 == 0) {
        if (colors_size / 3 == numpt) {
            color_channels = 3;
        }
        else if (colors_size / 4 == numpt) {
            color_channels = 4;
        }
        else {
            LOG_WARN("colors.size [{}] not match point size [{}].", colors_size,
                     numpt);
            return h;
        }
    }
    else {
        color_channels = colors_size % 3 == 0 ? 3 : 4;
    }

    const size_t numcl = colors_size / color_channels;

    if (numcl != 1 && numcl != numpt) {
        LOG_ERROR("color size [{}] not match point size [{}].", numcl, numpt);
        return h;
    }

    osg::ref_ptr<osg::Vec3Array> vs =
        new osg::Vec3Array(numpt, (const osg::Vec3 *)(xyzs.data()));

    osg::ref_ptr<osg::Array> cs;
    if (color_channels == 3) {
        cs = new osg::Vec3Array(numcl, (const osg::Vec3 *)(colors.data()));
    }
    else {
        cs = new osg::Vec4Array(numcl, (const osg::Vec4 *)(colors.data()));
    }

    osg::ref_ptr<osg::Geometry> geo = new osg::Geometry;
    geo->setVertexArray(vs.get());
    geo->setColorArray(cs.get());
    geo->setColorBinding(numcl == numpt ? osg::Geometry::BIND_PER_VERTEX
                                        : osg::Geometry::BIND_OVERALL);

    geo->addPrimitiveSet(
        new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, numpt));
    geo->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geo->getOrCreateStateSet()->setAttribute(new osg::Point(size),
                                             osg::StateAttribute::ON);
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geo.get());
    mt->addChild(geode);
    h.type = ViewObjectType_Point;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Line(const std::vector<float> &lines, float size,
                  const std::vector<float> &colors, int mode)
{
    Handle h;
    const size_t lines_size = lines.size();
    const size_t colors_size = colors.size();

    if (size <= 0) {
        LOG_WARN("line size is wrong! {0}", size);
        return h;
    }

    if (colors_size == 0 || (colors_size % 3 != 0 && colors_size % 4 != 0)) {
        LOG_WARN("colors.size is wrong! {0}", colors_size);
        return h;
    }

    if (lines_size == 0) {
        LOG_WARN("lines.size() is wrong! {0}", lines_size);
        return h;
    }

    if (mode > 3 || mode < 0) {
    }

    int primitive_set_mode = osg::PrimitiveSet::LINES + mode;
    size_t numlines = 0;

    switch (primitive_set_mode) {
    case osg::PrimitiveSet::LINES:
        if (lines_size % 6 != 0) {
            LOG_WARN("lines.size() is wrong! {0}", lines_size);
            return h;
        }
        numlines = lines_size / 6;
        break;
    case osg::PrimitiveSet::LINE_STRIP:
        if (lines_size % 3 != 0 || lines_size / 3 < 2) {
            LOG_WARN("lines.size() is wrong! {0}", lines_size);
            return h;
        }
        numlines = lines_size / 3 - 1;
        break;
    case osg::PrimitiveSet::LINE_LOOP:
        if (lines_size % 3 != 0 || lines_size / 3 < 3) {
            LOG_WARN("lines.size() is wrong! {0}", lines_size);
            return h;
        }
        numlines = lines_size / 3;
        break;
    default:
        LOG_ERROR("line mode is wrong! {0}", mode);
        return h;
    }

    size_t color_channels = 0;
    if (colors_size % 3 == 0 && colors_size % 4 == 0) {
        if (colors_size / 3 == numlines) {
            color_channels = 3;
        }
        else if (colors_size / 4 == numlines) {
            color_channels = 4;
        }
        else {
            LOG_WARN("colors.size [{}] not match line size [{}].", colors_size,
                     numlines);
            return h;
        }
    }
    else {
        color_channels = colors_size % 3 == 0 ? 3 : 4;
    }
    size_t numcolors = colors_size / color_channels;

    if (numcolors != 1 && numcolors != numlines) {
        LOG_WARN("Color number should be 1 or the same with line number! [{}] "
                 "!= [{}]",
                 numcolors, numlines);
        return h;
    }

    std::vector<float> vert_colors;
    if (numcolors == 1 || primitive_set_mode != osg::PrimitiveSet::LINES) {
        vert_colors = colors;
        numcolors = 1;
    }
    else {
        vert_colors.resize(colors_size * 2);
        for (size_t i = 0; i < numcolors; ++i) {
            for (size_t j = 0; j < color_channels; ++j) {
                vert_colors[i * 2 * color_channels + j] =
                    colors[i * color_channels + j];
                vert_colors[(i * 2 + 1) * color_channels + j] =
                    colors[i * color_channels + j];
            }
        }
        numcolors *= 2;
    }

    osg::ref_ptr<osg::Vec3Array> vs =
        new osg::Vec3Array(lines.size() / 3, (const osg::Vec3 *)(lines.data()));

    osg::ref_ptr<osg::Array> cs;
    if (color_channels == 3) {
        cs = new osg::Vec3Array(numcolors,
                                (const osg::Vec3 *)(vert_colors.data()));
    }
    else {
        cs = new osg::Vec4Array(numcolors,
                                (const osg::Vec4 *)(vert_colors.data()));
    }

    osg::ref_ptr<osg::Geometry> geo = new osg::Geometry;
    geo->setVertexArray(vs.get());
    geo->setColorArray(cs.get());
    geo->setColorBinding(numcolors == 1 ? osg::Geometry::BIND_OVERALL
                                        : osg::Geometry::BIND_PER_VERTEX);

    geo->addPrimitiveSet(
        new osg::DrawArrays(primitive_set_mode, 0, lines.size() / 3));
    geo->getOrCreateStateSet()->setAttribute(new osg::LineWidth(size),
                                             osg::StateAttribute::ON);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING,
                                          osg::StateAttribute::OFF);
    geode->addDrawable(geo);

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    geode->addDrawable(geo.get());
    mt->addChild(geode);
    h.type = ViewObjectType_Line;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

static bool GenerateGridMesh(float xlenth, float ylenth, int half_x_num_cells,
                             int half_y_num_cells, std::vector<float> &vertices,
                             std::vector<unsigned int> &indices)
{
    if (xlenth <= 0 || ylenth <= 0 || half_x_num_cells < 1
        || half_y_num_cells < 1) {
        LOG_ERROR("Invalid parameters! All parameters should be positive. {}, "
                  "{}, {}, {}",
                  xlenth, ylenth, half_x_num_cells, half_y_num_cells);
        return false;
    }

    const int64_t max_cell_num = 40000000;
    int64_t all_num_cells =
        (int64_t)half_x_num_cells * (int64_t)half_y_num_cells * 4;

    if (all_num_cells > max_cell_num) {
        LOG_ERROR("To much cells! Can not handle {} cells.", all_num_cells);
        return false;
    }

    vertices.clear();
    indices.clear();

    int x_vertices_num = half_x_num_cells * 2 + 1;
    int y_vertices_num = half_y_num_cells * 2 + 1;
    int vertices_num = x_vertices_num * y_vertices_num;
    vertices.resize((size_t)vertices_num * 3);
    indices.resize(2 * (size_t)half_x_num_cells * 2 * half_y_num_cells * 2 * 3);

    size_t pi = 0;
    float x_cell_size = xlenth / (half_x_num_cells * 2);
    float y_cell_size = ylenth / (half_y_num_cells * 2);

    // generate vertices
    for (int y = -half_y_num_cells; y <= half_y_num_cells; ++y) {
        for (int x = -half_x_num_cells; x <= half_x_num_cells; ++x) {
            vertices[pi * 3 + 0] = x * x_cell_size;
            vertices[pi * 3 + 1] = y * y_cell_size;
            vertices[pi * 3 + 2] = 0;
            ++pi;
        }
    }

    // assign indices in anti-clockwise order
    pi = 0;
    for (int j = 0; j < y_vertices_num - 1; j++) {
        for (int i = 0; i < x_vertices_num - 1; ++i) {
            int row1 = j * x_vertices_num;
            int row2 = (j + 1) * x_vertices_num;
            indices[pi++] = row1 + i;
            indices[pi++] = row2 + i + 1;
            indices[pi++] = row2 + i;
            indices[pi++] = row1 + i;
            indices[pi++] = row1 + i + 1;
            indices[pi++] = row2 + i + 1;
        }
    }
    return true;
}

static void TransformVertices(const std::array<float, 3> &trans,
                              const std::array<float, 9> &rotation,
                              std::vector<float> &pts)
{
    size_t npts = pts.size() / 3;
    for (size_t i = 0; i < npts; ++i) {
        const float x = pts[i * 3 + 0];
        const float y = pts[i * 3 + 1];
        const float z = pts[i * 3 + 2];
        pts[i * 3 + 0] =
            rotation[0] * x + rotation[1] * y + rotation[2] * z + trans[0];
        pts[i * 3 + 1] =
            rotation[3] * x + rotation[4] * y + rotation[5] * z + trans[1];
        pts[i * 3 + 2] =
            rotation[6] * x + rotation[7] * y + rotation[8] * z + trans[2];
    }
}

static void Quaternion2Matrix(const std::array<float, 4> &quat,
                              std::array<float, 9> &R)
{
    float x = quat[0];
    float y = quat[1];
    float z = quat[2];
    float w = quat[3];

    float invNorm;
    invNorm = 1.0f / (float)std::sqrt(w * w + x * x + y * y + z * z);

    w *= invNorm;
    x *= invNorm;
    y *= invNorm;
    z *= invNorm;

    R[0] = 1 - 2 * y * y - 2 * z * z;
    R[1] = 2 * x * y - 2 * w * z;
    R[2] = 2 * x * z + 2 * w * y;
    R[3] = 2 * x * y + 2 * w * z;
    R[4] = 1 - 2 * x * x - 2 * z * z;
    R[5] = 2 * y * z - 2 * w * x;
    R[6] = 2 * x * z - 2 * w * y;
    R[7] = 2 * y * z + 2 * w * x;
    R[8] = 1 - 2 * x * x - 2 * y * y;
}

Handle View::Box(const std::array<float, 3> &pos,
                 const std::array<float, 3> &extents,
                 const std::vector<float> &color)
{
    Handle h;
    const int color_size = color.size();
    if (color_size != 3 && color_size != 4) {
        LOG_WARN("color_size should be 3 or 4!");
        return h;
    }

    for (const auto e : extents) {
        if (e <= 0) {
            LOG_WARN("Invalid extent for box: {0}.", e);
            return h;
        }
    }

    const bool transparent = color_size == 3 ? false : true;

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode{new osg::Geode()};

    osg::ref_ptr<osg::Box> box{new osg::Box()};

    box->setHalfLengths(osg::Vec3(extents[0], extents[1], extents[2]));

    osg::ref_ptr<osg::ShapeDrawable> sd{new osg::ShapeDrawable(box.get())};
    sd->setColor(osg::Vec4(color[0], color[1], color[2],
                           color_size == 3 ? 1.f : color[3]));

    if (transparent) {
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }

    geode->addDrawable(sd);
    mt->addChild(geode);

    h.type = ViewObjectType_Box;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    osg::Matrixf m;
    m.setTrans(pos[0], pos[1], pos[2]);
    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Cylinder(const std::array<float, 3> &center, float radius,
                      float height, const std::vector<float> &color)
{
    Handle h;
    const int color_size = color.size();
    if (radius <= 0 || height <= 0) {
        LOG_WARN("Invalid radius or height: {0}, {1}.", radius, height);
        return h;
    }

    if (color_size != 3 && color_size != 4) {
        LOG_WARN("color_size should be 3 or 4!");
        return h;
    }

    const bool transparent = color_size == 3 ? false : true;

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode{new osg::Geode()};

    osg::ref_ptr<osg::Cylinder> cylinder{new osg::Cylinder()};

    // cylinder->setCenter(osg::Vec3f(pos[0], pos[1], pos[2]));
    cylinder->setRadius(radius);
    cylinder->setHeight(height);

    osg::ref_ptr<osg::ShapeDrawable> sd{new osg::ShapeDrawable(cylinder.get())};
    sd->setColor(osg::Vec4(color[0], color[1], color[2],
                           color_size == 3 ? 1.f : color[3]));

    if (transparent) {
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }

    geode->addDrawable(sd);
    mt->addChild(geode);

    h.type = ViewObjectType_Cylinder;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    osg::Matrixf m;
    m.setTrans(center[0], center[1], center[2]);
    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Sphere(const std::array<float, 3> &center, float radius,
                    const std::vector<float> &color)
{
    Handle h;
    const int color_size = color.size();
    if (radius <= 0) {
        LOG_WARN("radius should be positive {0}.", radius);
        return h;
    }

    if (color_size != 3 && color_size != 4) {
        LOG_WARN("color_size should be 3 or 4!");
        return h;
    }

    const bool transparent = color.size() == 3 ? false : true;

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode{new osg::Geode()};

    osg::ref_ptr<osg::Sphere> sphere{new osg::Sphere()};

    // sphere->setCenter(osg::Vec3f(pos[0], pos[1], pos[2]));
    sphere->setRadius(radius);

    osg::ref_ptr<osg::ShapeDrawable> sd{new osg::ShapeDrawable(sphere.get())};
    sd->setColor(osg::Vec4(color[0], color[1], color[2],
                           color.size() == 3 ? 1.f : color[3]));

    if (transparent) {
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }

    geode->addDrawable(sd);
    mt->addChild(geode);

    h.type = ViewObjectType_Sphere;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    osg::Matrixf m;
    m.setTrans(center[0], center[1], center[2]);
    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Spheres(const std::vector<float> &centers,
                     std::vector<float> &radii,
                     const std::vector<float> &colors)
{
    Handle h;
    const int centers_size = (int)centers.size();
    const int radii_size = (int)radii.size();
    const int colors_size = (int)colors.size();

    if (centers_size == 0 || centers_size % 3 != 0) {
        LOG_WARN("centers.size() is wrong! {0}", centers_size);
        return h;
    }

    const int num_spheres = centers_size / 3;

    if (radii_size == 0 || (radii_size != 1 && radii_size != num_spheres)) {
        LOG_WARN("radii.size() is wrong! {0}", radii_size);
        return h;
    }

    for (auto &radius : radii) {
        if (radius <= 0) {
            LOG_WARN("radius is wrong! {0}", radius);
            return h;
        }
    }

    if (colors_size == 0 || colors_size % 4 != 0
        || (colors_size / 4 != num_spheres && colors_size != 4)) {
        LOG_WARN("colors.size is wrong! {0}", colors_size);
        return h;
    }

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode{new osg::Geode()};

    for (int i = 0; i < num_spheres; ++i) {
        osg::ref_ptr<osg::Sphere> sphere{new osg::Sphere()};
        double radius = (radii_size == 1) ? radii[0] : radii[i];
        sphere->setCenter(osg::Vec3f(centers[0 + 3 * i], centers[1 + 3 * i],
                                     centers[2 + 3 * i]));
        sphere->setRadius(radius);
        osg::ref_ptr<osg::ShapeDrawable> sd{
            new osg::ShapeDrawable(sphere.get())};
        int offset_index = (colors_size == 1) ? 0 : i;
        sd->setColor(osg::Vec4(
            colors[0 + 4 * offset_index], colors[1 + 4 * offset_index],
            colors[2 + 4 * offset_index], colors[3 + 4 * offset_index]));

        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
        geode->addDrawable(sd);
        mt->addChild(geode);
    }

    h.type = ViewObjectType_Spheres;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Cone(const std::array<float, 3> &center, float radius,
                  float height, const std::vector<float> &color)
{
    /// NOTE: Center of cone is at the 1/4 height from bottom place inside the
    /// cone
    Handle h;
    const int color_size = color.size();
    if (radius <= 0 || height <= 0) {
        LOG_WARN("Invalid radius or height: {0}, {1}.", radius, height);
        return h;
    }

    if (color_size != 3 && color_size != 4) {
        LOG_WARN("color_size should be 3 or 4!");
        return h;
    }

    const bool transparent = color.size() == 3 ? false : true;

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode{new osg::Geode()};

    osg::ref_ptr<osg::Cone> cone{new osg::Cone()};

    // cone->setCenter(osg::Vec3f(pos[0], pos[1], pos[2]));
    cone->setRadius(radius);
    cone->setHeight(height);

    osg::ref_ptr<osg::ShapeDrawable> sd{new osg::ShapeDrawable(cone.get())};
    sd->setColor(osg::Vec4(color[0], color[1], color[2],
                           color.size() == 3 ? 1.f : color[3]));

    if (transparent) {
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }

    geode->addDrawable(sd);
    mt->addChild(geode);

    h.type = ViewObjectType_Cone;
    h.uid = NextHandleID();

    geode->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    osg::Matrixf m;
    m.setTrans(center[0], center[1], center[2]);
    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});

    return h;
}

/** Plot an arrow. The tail is the bottom, the head is the sharp side.
 *
 * We use a cylinder and a cone to compose an arrow.
 */
Handle View::Arrow(const std::array<float, 3> &tail,
                   const std::array<float, 3> &head, float radius,
                   const std::vector<float> &color)
{
    Handle h;
    const int color_size = color.size();
    if (radius <= 0) {
        LOG_WARN("Invalid radius: {0}.", radius);
        return h;
    }

    if (color_size != 3 && color_size != 4) {
        LOG_WARN("color_size should be 3 or 4!");
        return h;
    }

    const bool transparent = color.size() == 3 ? false : true;

    // Arrow is composed by cone and cylinder
    const osg::Vec3f tail_{tail[0], tail[1], tail[2]};
    const osg::Vec3f head_{head[0], head[1], head[2]};
    const float cone_ratio = 0.2f;
    osg::Vec3f vec = head_ - tail_;
    const float vec_len = vec.normalize();
    const float cone_height = vec_len * cone_ratio;
    const float cylinder_height = vec_len - cone_height;

    osg::Quat quat;
    const osg::Vec3f zaxis{0, 0, 1};
    quat.makeRotate(zaxis, vec);

    osg::ref_ptr<osg::Cone> cone{new osg::Cone()};
    osg::ref_ptr<osg::Cylinder> cylinder{new osg::Cylinder()};

    cylinder->setCenter(osg::Vec3f(0, 0, cylinder_height * 0.5f));
    cylinder->setRadius(radius);
    cylinder->setHeight(cylinder_height);
    cone->setCenter(osg::Vec3f(
        0, 0, cylinder_height + cone_height * 0.25f)); // Move cone to the top
    cone->setRadius(radius);
    cone->setHeight(cone_height);

    osg::ref_ptr<osg::ShapeDrawable> sd_cone{
        new osg::ShapeDrawable(cone.get())};
    sd_cone->setColor(osg::Vec4(color[0], color[1], color[2],
                                color.size() == 3 ? 1.f : color[3]));

    osg::ref_ptr<osg::ShapeDrawable> sd_cylinder{
        new osg::ShapeDrawable(cylinder.get())};
    sd_cylinder->setColor(osg::Vec4(color[0], color[1], color[2],
                                    color.size() == 3 ? 1.f : color[3]));

    if (transparent) {
        sd_cylinder->getOrCreateStateSet()->setMode(GL_BLEND,
                                                    osg::StateAttribute::ON);
        sd_cylinder->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
        sd_cone->getOrCreateStateSet()->setMode(GL_BLEND,
                                                osg::StateAttribute::ON);
        sd_cone->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode_cone{new osg::Geode()};
    osg::ref_ptr<osg::Geode> geode_cylinder{new osg::Geode()};

    geode_cone->addDrawable(sd_cone);
    geode_cylinder->addDrawable(sd_cylinder);

    mt->addChild(geode_cone);
    mt->addChild(geode_cylinder);

    h.type = ViewObjectType_Arrow;
    h.uid = NextHandleID();

    geode_cone->setName(std::to_string(NextObjectID()));
    geode_cylinder->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    osg::Matrixf m;
    m.setTrans(tail[0], tail[1], tail[2]);
    m.setRotate(quat);
    mt->setMatrix(m);
    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Mesh(const std::vector<float> &vertices,
                  const std::vector<unsigned int> &indices,
                  const std::vector<float> &colors)
{
    Handle h;
    const size_t vertices_size = vertices.size();
    const size_t indices_size = indices.size();
    const size_t colors_size = colors.size();
    const size_t numverts = vertices_size / 3;
    if (vertices_size == 0 || vertices_size % 3 != 0) {
        LOG_WARN("vertices.size() is wrong! {0}", vertices_size);
        return h;
    }
    if (indices_size == 0 || indices_size % 3 != 0) {
        LOG_WARN("indices.size() is wrong! {0}", indices_size);
        return h;
    }
    if (colors_size == 0 || (colors_size % 3 != 0 && colors_size % 4 != 0)) {
        LOG_WARN("colors.size is wrong! {0}", colors_size);
        return h;
    }

    size_t color_channels = 0;
    if (colors_size % 3 == 0 && colors_size % 4 == 0) {
        if (colors_size / 3 == numverts) {
            color_channels = 3;
        }
        else if (colors_size / 4 == numverts) {
            color_channels = 4;
        }
        else {
            LOG_WARN("colors.size [{}] not match vertices size [{}].",
                     colors_size, numverts);
            return h;
        }
    }
    else {
        color_channels = colors_size % 3 == 0 ? 3 : 4;
    }
    const size_t numcolors = colors_size / color_channels;

    if (numcolors != 1 && numcolors != numverts) {
        LOG_WARN("Color number should be 1 or the same with points number! "
                 "[{}] != [{}]",
                 numcolors, numverts);
        return h;
    }

    osg::ref_ptr<osg::Array> cs;
    if (color_channels == 3) {
        cs = new osg::Vec3Array(numcolors, (const osg::Vec3 *)(colors.data()));
    }
    else {
        cs = new osg::Vec4Array(numcolors, (const osg::Vec4 *)(colors.data()));
    }
    osg::ref_ptr<osg::Vec3Array> ref_vertices =
        new osg::Vec3Array(vertices_size, (const osg::Vec3 *)(vertices.data()));
    osg::ref_ptr<osg::DrawElementsUInt> ref_indices =
        new osg::DrawElementsUInt(GL_TRIANGLES, indices_size, indices.data());
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(ref_vertices.get());
    geom->addPrimitiveSet(ref_indices.get());
    geom->setColorArray(cs.get());
    geom->setColorBinding(numcolors == 1 ? osg::Geometry::BIND_OVERALL
                                         : osg::Geometry::BIND_PER_VERTEX);
    osgUtil::SmoothingVisitor::smooth(*geom);

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode_mesh{new osg::Geode()};
    geode_mesh->addDrawable(geom.get());
    mt->addChild(geode_mesh);

    h.type = ViewObjectType_Mesh;
    h.uid = NextHandleID();

    geode_mesh->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

Handle View::Plane(float xlength, float ylength, int half_x_num_cells,
                   int half_y_num_cells, const std::vector<float> &color)
{
    return Plane(xlength, ylength, half_x_num_cells, half_y_num_cells,
                 {0, 0, 0}, {0, 0, 0, 1}, color);
}

Handle View::Plane(float xlength, float ylength, int half_x_num_cells,
                   int half_y_num_cells, const std::array<float, 3> &trans,
                   const std::array<float, 4> &quat,
                   const std::vector<float> &color)
{
    Handle h;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::array<float, 9> rotation;

    if (color.size() != 3 && color.size() != 4) {
        LOG_WARN("color.size() should be 3 or 4!");
        return h;
    }

    if (quat[0] == 0 && quat[1] == 0 && quat[2] == 0 && quat[3] == 0) {
        LOG_WARN("quanternion (0, 0, 0, 0) is not valid!");
        return h;
    }

    Quaternion2Matrix(quat, rotation);

    if (!GenerateGridMesh(xlength, ylength, half_x_num_cells, half_y_num_cells,
                          vertices, indices)) {
        return h;
    }

    TransformVertices(trans, rotation, vertices);

    const int color_channels = color.size() % 3 == 0 ? 3 : 4;
    osg::ref_ptr<osg::Array> cs;
    bool transparent = false;
    if (color_channels == 3) {
        cs = new osg::Vec3Array(1, (const osg::Vec3 *)(color.data()));
    }
    else {
        cs = new osg::Vec4Array(1, (const osg::Vec4 *)(color.data()));
        if (color[3] != 1.0f) {
            transparent = true;
        }
    }
    osg::ref_ptr<osg::Vec3Array> ref_vertices = new osg::Vec3Array(
        vertices.size() / 3, (const osg::Vec3 *)(vertices.data()));
    osg::ref_ptr<osg::DrawElementsUInt> ref_indices =
        new osg::DrawElementsUInt(GL_TRIANGLES, indices.size(), indices.data());
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(ref_vertices.get());
    geom->addPrimitiveSet(ref_indices.get());
    geom->setColorArray(cs.get());
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    if (transparent) {
        geom->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        geom->getOrCreateStateSet()->setRenderingHint(
            osg::StateSet::TRANSPARENT_BIN);
    }
    osgUtil::SmoothingVisitor::smooth(*geom);

    osg::ref_ptr<osg::MatrixTransform> mt{new osg::MatrixTransform};
    osg::ref_ptr<osg::Geode> geode_mesh{new osg::Geode()};
    geode_mesh->addDrawable(geom.get());
    mt->addChild(geode_mesh);

    h.type = ViewObjectType_Plane;
    h.uid = NextHandleID();

    geode_mesh->setName(std::to_string(NextObjectID()));
    mt->setName(std::string{"mt"} + std::to_string(NextObjectID()));

    m_vis3d->node_switch->addChild(mt);
    m_vis3d->node_map.insert({h, mt});
    return h;
}

void View::SetIntersectMode(IntersectorMode mode, bool hover)
{
    m_vis3d->insector_mode = mode;
    m_vis3d->insector_hover = hover;
}

bool View::EnableGizmo(const Handle &h, int gizmotype)
{
    if (!Vis3d__HasNode(m_vis3d, h)) {
        LOG_ERROR("Can not find node: type: {0}, uid: {1}.", h.type, h.uid);
        return false;
    }
    if (gizmotype < 1 || gizmotype > 4) {
        LOG_ERROR("operation type should be 1 to 3, current is {}.", gizmotype);
        return false;
    }

    // Disable last Gizmo
    DisableGizmo();

    auto mt = m_vis3d->node_map[h];
    const osg::Matrix &matrix = mt->getMatrix();
    for (int i = 0; i < 16; ++i) {
        m_vis3d->gizmo.matrix[i] = *(matrix.ptr() + i);
    }
    auto vp = m_vis3d->osgviewer->getCamera()->getViewport();

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    root->addChild(geode.get());
    geode->setCullingActive(false); // allow gizmo to always display
    geode->getOrCreateStateSet()->setRenderingHint(
        osg::StateSet::TRANSPARENT_BIN); // always show at last

    if (gizmotype == 4) {
        osg::ref_ptr<GizmoDrawable> gizmo = new GizmoDrawable;
        gizmo->setCaptureFlag(&m_vis3d->gizmo.capture);
        gizmo->setTransform(mt.get(), m_vis3d->gizmo.matrix);
        gizmo->setGizmoMode(GizmoDrawable::MOVE_GIZMO);
        gizmo->setScreenSize(vp->width(), vp->height());
        geode->addDrawable(gizmo.get());

        gizmo = new GizmoDrawable;
        gizmo->setCaptureFlag(&m_vis3d->gizmo.capture);
        gizmo->setTransform(mt.get(), m_vis3d->gizmo.matrix);
        gizmo->setGizmoMode(GizmoDrawable::ROTATE_GIZMO);
        gizmo->setScreenSize(vp->width(), vp->height());
        geode->addDrawable(gizmo.get());
    }
    else {
        osg::ref_ptr<GizmoDrawable> gizmo = new GizmoDrawable;
        gizmo->setCaptureFlag(&m_vis3d->gizmo.capture);
        gizmo->setTransform(mt.get(), m_vis3d->gizmo.matrix);
        gizmo->setGizmoMode((GizmoDrawable::Mode)gizmotype);
        gizmo->setScreenSize(vp->width(), vp->height());
        geode->addDrawable(gizmo.get());
    }

    m_vis3d->gizmo.refHandle = h;
    m_vis3d->gizmo.handle.uid = NextHandleID();
    m_vis3d->gizmo.handle.type = ViewObjectType_Gzimo;
    m_vis3d->gizmo.capture = false;

    m_vis3d->node_switch->addChild(root);
    m_vis3d->node_map[m_vis3d->gizmo.handle] = root;

    return true;
}

bool View::SetGizmoType(int gizmotype, int loctype)
{
    LOG_DEBUG("gizmo type: {}, location type : {}.", gizmotype, loctype);
    if (gizmotype < 1 || gizmotype > 3) {
        LOG_ERROR("operation type should be 1 to 3, current is {}.", gizmotype);
        return false;
    }
    if (!(m_vis3d->gizmo.handle.uid)) {
        LOG_ERROR("Not enable Gizmo!");
        return false;
    }
    osg::Geode *geode =
        m_vis3d->node_map[m_vis3d->gizmo.handle]->getChild(0)->asGeode();
    if (geode->getNumDrawables() > 1) {
        LOG_WARN("No need change type in all mode.");
        return false;
    }
    GizmoDrawable *gizmo = dynamic_cast<GizmoDrawable *>(geode->getDrawable(0));
    GizmoDrawable::Mode m = (GizmoDrawable::Mode)gizmotype;

    gizmo->setGizmoMode(m);
    return true;
}

bool View::SetGizmoDrawMask(int gizmotype, int mask)
{
    if (gizmotype < 1 || gizmotype > 3) {
        LOG_ERROR("operation type should be 1 to 3, current is {}.", gizmotype);
        return false;
    }
    if (!(m_vis3d->gizmo.handle.uid)) {
        LOG_ERROR("Not enable Gizmo!");
        return false;
    }
    osg::Geode *geode =
        m_vis3d->node_map[m_vis3d->gizmo.handle]->getChild(0)->asGeode();
    for (unsigned int i = 0; i < geode->getNumDrawables(); i++) {
        GizmoDrawable *gizmo =
            dynamic_cast<GizmoDrawable *>(geode->getDrawable(i));
        if (gizmo->getGizmoMode() == gizmotype) {
            gizmo->setDrawMask(mask);
            break;
        }
    }
    return true;
}

bool View::SetGizmoDisplayScale(float scale)
{
    if (scale <= 0.0f) {
        LOG_ERROR("scale is out of range.");
        return false;
    }
    if (!(m_vis3d->gizmo.handle.uid)) {
        LOG_ERROR("Not enable Gizmo!");
        return false;
    }
    osg::Geode *geode =
        m_vis3d->node_map[m_vis3d->gizmo.handle]->getChild(0)->asGeode();
    for (unsigned int i = 0; i < geode->getNumDrawables(); i++) {
        GizmoDrawable *gizmo =
            dynamic_cast<GizmoDrawable *>(geode->getDrawable(i));
        gizmo->setDisplayScale(scale);
    }
    return true;
}

bool View::SetGizmoDetectionRange(float range)
{
    if (range <= 0.0f || range >= 1.0f) {
        LOG_ERROR("detection range is out of range.");
        return false;
    }
    if (!(m_vis3d->gizmo.handle.uid)) {
        LOG_ERROR("Not enable Gizmo!");
        return false;
    }
    osg::Geode *geode =
        m_vis3d->node_map[m_vis3d->gizmo.handle]->getChild(0)->asGeode();
    for (unsigned int i = 0; i < geode->getNumDrawables(); i++) {
        GizmoDrawable *gizmo =
            dynamic_cast<GizmoDrawable *>(geode->getDrawable(i));
        gizmo->setDetectionRange(range);
    }
    return true;
}

bool View::DisableGizmo()
{
    if (m_vis3d->gizmo.handle.uid == 0) {
        LOG_INFO("Already disable gizmo.");
        return true;
    }
    m_vis3d->gizmo.refHandle = Handle();
    m_vis3d->gizmo.capture = false;
    Delete(m_vis3d->gizmo.handle);
    m_vis3d->gizmo.handle = Handle();
    return true;
}

bool View::EnableShortcutKey()
{
    // auto osgview = m_vis3d->compviewer->getView(0);
    // bool enable = false;
    // for (auto ev : osgview->getEventHandlers()) {
    //     if (ev == m_vis3d->statshandler) {
    //         enable = true;
    //         break;
    //     }
    // }
    // if (enable) {
    //     m_vis3d->statshandler->getCamera()->setNodeMask(0x0);
    //     m_vis3d->helphandler->getCamera()->setNodeMask(0x0);
    //     osgview->removeEventHandler(m_vis3d->statshandler);
    //     osgview->removeEventHandler(m_vis3d->helphandler);
    //     osgview->removeEventHandler(m_vis3d->wshandler);
    //     osgview->removeEventHandler(m_vis3d->ssmhandler);
    // }
    // else {
    //     m_vis3d->statshandler->getCamera()->setNodeMask(0xffffffff);
    //     m_vis3d->helphandler->getCamera()->setNodeMask(0xffffffff);
    //     osgview->addEventHandler(m_vis3d->statshandler);
    //     osgview->addEventHandler(m_vis3d->helphandler);
    //     osgview->addEventHandler(m_vis3d->wshandler);
    //     osgview->addEventHandler(m_vis3d->ssmhandler);
    // }
    // return !enable;
    return false;
}

bool View::EnableTrackballManipulation(bool enable)
{
    return m_vis3d->gizmo.view_manipulation = !enable;
}

osgViewer::Viewer *View::GetOsgViewer() { return m_vis3d->osgviewer; }