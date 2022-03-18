// Copyright (c) RVBUST, Inc - All rights reserved.
#pragma once

#include <osg/Matrix>
#include <osgViewer/Viewer>
#include <osgFX/Outline>

#include <stdint.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace Vis
{

class QViewerWidget;


enum ViewObjectType
{
    ViewObjectType_None = 0,
    ViewObjectType_View,
    ViewObjectType_Model,
    ViewObjectType_Point,
    ViewObjectType_Line,
    ViewObjectType_Arrow,
    ViewObjectType_Mesh,
    ViewObjectType_Axes,
    ViewObjectType_Box,
    ViewObjectType_Plane,
    ViewObjectType_Sphere,
    ViewObjectType_Spheres,
    ViewObjectType_Cone,
    ViewObjectType_Cylinder,
    ViewObjectType_Gzimo,
};

// clang-format off
enum IntersectorMode {
    IntersectorMode_Disable = 0,  // Disable picking mode
    IntersectorMode_Polytope,     // used to pick object (including point, line, etc.), cann't calculate intersection point
    IntersectorMode_LineSegment,  // used to pick a point and object in scene, hard to pick point, line
    IntersectorMode_Point,        // used to pick a point from a point cloud (drawn with viewer.Point(...))
    IntersectorMode_Line,         // used to pick a line node in scene (drawn with viewer.Line(...))
};
// clang-format on

struct Handle
{
    Handle() : type(0), uid(0) {}
    Handle(uint64_t t, uint64_t u) : type(t), uid(u) {}
    inline void Reset()
    {
        type = 0;
        uid = 0;
    }
    uint64_t type;
    uint64_t uid;
};

inline bool operator==(const Handle a, const Handle b)
{
    return a.type == b.type && a.uid == b.uid;
}

struct HandleHasher
{
    inline std::size_t operator()(const Handle h) const
    {
        return std::hash<decltype(h.type)>()(h.type)
               ^ std::hash<decltype(h.uid)>()(h.uid);
    }
};

struct VisGizmo
{
    int capture{0};
    bool view_manipulation{false};
    Handle handle;
    Handle refHandle;
    float matrix[16];
};

struct Vis3d
{
    bool is_inited{false};

    VisGizmo gizmo;
    bool done;
    Handle picked;
    std::array<float, 6> pointnorm{0};
    std::unordered_map<Handle, osg::ref_ptr<osg::MatrixTransform>, HandleHasher>
        node_map;
    std::unordered_map<Handle, osg::ref_ptr<osgFX::Outline>, HandleHasher>
        outlinemap;

    osg::ref_ptr<osg::Group> scene_root;
    osg::ref_ptr<osg::Switch> node_switch;
    osg::ref_ptr<osgViewer::Viewer> osgviewer;

    IntersectorMode insector_mode;
    bool insector_hover{false};
};

struct View
{
    friend class QViewerWidget;

    View(osgViewer::Viewer *viewer = nullptr);

    /**
     * Close the window.
     * Return true if succeed else false.
     */
    bool Close(); // Close the View

    /**
     * Enter the main loop to process frame
     */
    int Loop();

    /**
     * Get window size
     * Return the size of the window
     */
    std::pair<float, float> GetViewSize();

    /**
     * @brief Set/Get camera's pose by using eye/center/up vectors
     *
     * @return bool True if succeed, else False
     */
    bool SetCameraPose(const std::array<float, 3> &eye,
                       const std::array<float, 3> &point_want_to_look,
                       const std::array<float, 3> &upvector);
    bool GetCameraPose(std::array<float, 3> &eye,
                       std::array<float, 3> &point_want_to_look,
                       std::array<float, 3> &upvector,
                       float look_distance = 1.0f);

    /**
     * @brief Set/Get The view camera's home pose.
     * The home pose is the default pose when you press space key.
     *
     * @return bool True if succeed, else False
     */
    bool SetHomePose(const std::array<float, 3> &eye,
                     const std::array<float, 3> &point_want_to_look,
                     const std::array<float, 3> &upvector);
    bool GetHomePose(std::array<float, 3> &eye,
                     std::array<float, 3> &point_want_to_look,
                     std::array<float, 3> &upvector);

    /**
     * Delete an node from the scene.
     * Return true if succeed else false.
     */
    bool Delete(const Handle &nh); // Delete node

    bool IsClosed() const;

    bool IsAlive(const Handle &nh) const;

    /**
     * Delete nodes from the scene
     * Return true if all deleted else false and top at the break point.
     * NOTE: This function simply calls Delet(handle), so it will be slow since
     * it's not operating in batch mode.
     */
    bool Delete(const std::vector<Handle> &handles); // delete list of handles

    /**
     * Clear all nodes in the scene.
     */
    bool Clear(); // Delete all nodes

    /**
     * Go to home position
     */
    bool Home();

    /**
     * Switch on the node to make it rendered and visible.
     */
    bool Show(const Handle &nh);

    /**
     * Switch off the node to make it invisible.
     */
    bool Hide(const Handle &nh);

    /**
     * Chain a list of links together: links[0] -> links[1] ...
     * The first link will be the parent of the second link.
     * All children links (start from the second) will be removed from their
     * parents first, then link together. Note: the same links should not be
     * passed into this function, since it will form a circle, which will break
     * the graph. Return true of succeed else false.
     */
    bool Chain(const std::vector<Handle> &links);

    /**
     * Unchain a set of linked objects.
     */
    bool Unchain(const std::vector<Handle> &links);

    /**
     * Set transforms for each object indepedently. If there're links between
     * objects, the transformation will be accumulated.
     */
    bool SetTransforms(const std::vector<Handle> &hs,
                       const std::vector<std::array<float, 3>> &trans,
                       const std::vector<std::array<float, 4>> &quats);

    /**
     * Set the transformations from a base node and it's decendents. In this
     * case, each parent should only have single decedent.
     */
    bool SetTransforms(const Handle &h,
                       const std::vector<std::array<float, 3>> &trans,
                       const std::vector<std::array<float, 4>> &quats);

    /**
     * Set the transparency level of an object.
     * @param inv_alpha is 1 - alpha.
     */
    bool SetTransparency(const Handle &nh, float inv_alpha);

    // bool SetName(const Handle &nh, const std::string &name);

    /**
     * Highlighting a model
     *
     * Highlight a model with given handle
     *
     * @code
     * bool b = v.SetOutline(h, true, 8, {1, 0, 0, 1});
     * bool b2 = v.SetOutline({h1, h2}, true, 8, {0, 1, 0, 1});
     * @endcode
     * @param show   show outline if true
     * @param width  set width of the highlight
     * @param color  set color of the highlight
     * @return true if suceess
     */
    bool ShowOutline(const Handle &nh, bool show, float width = 8,
                     const std::vector<float> &color = {1.0f, 0.0f, 0.0f});

    bool ShowOutline(const std::vector<Handle> &hs, bool show, float width = 8,
                     const std::vector<float> &color = {1.0f, 0.0f, 0.0f});

    /**
     * Clone a node by handle. Return a new to handle to the cloned object.
     */
    Handle Clone(const Handle h);

    Handle Clone(const Handle h, const std::array<float, 3> &pos,
                 const std::array<float, 4> &quat);

    // NOTE: the input handles should not be chained together !!
    std::vector<Handle> Clone(const std::vector<Handle> &handles);

    std::vector<Handle> Clone(const std::vector<Handle> &handles,
                              const std::vector<std::array<float, 3>> &poss,
                              const std::vector<std::array<float, 4>> &quats);

    // bool ShowAxes(bool show = true);
    Handle Axes(const std::array<float, 3> &trans = {0.f, 0.f, 0.f},
                const std::array<float, 4> &quat = {0.f, 0.f, 0.f, 1.f},
                float axis_len = 15.f, float axis_size = 3.f);
    Handle Axes(const std::array<float, 16> &transform, float axis_len = 15.f,
                float axis_size = 3.f);

    /**
     * Return the latest picked object's handle.
     */
    Handle Picked();

    /**
     * Unpick picked object`s handle.
     */
    void UnPick();

    /**
     * Return point and norm in the latest picked object.
     */
    std::array<float, 6> PickedPlane();

    /**
     * Load model
     */
    Handle Load(const std::string &fname);
    std::vector<Handle> Load(const std::vector<std::string> &fnames);

    Handle Load(const std::string &fname, const std::array<float, 3> &pos,
                const std::array<float, 4> &quat);
    std::vector<Handle> Load(const std::vector<std::string> &fnames,
                             const std::vector<std::array<float, 3>> &trans,
                             const std::vector<std::array<float, 4>> &quats);

    Handle Point(const std::vector<float> &xyzs, float ptsize = 1.0f,
                 const std::vector<float> &colors = {1.f, 0.f, 0.f});

    /**
     * Plot line or lines
     *
     * We first plot the plane with the center at the world origin, then
     * transfrom it to the target place with provided T
     *
     * @code
     * h = v.Plane({0,0,0,1,0,0});
     * @endcode
     * @param lines points of the line or lines
     * @param size width of the line or lines
     * @param color color of the line or lines
     *         only LINES can set color per line
     * @param mode 0 for LINES 1 for LINE_LOOP 2 for LINE_STRIP
     * @return Handle
     */
    Handle Line(const std::vector<float> &lines, float size = 1.f,
                const std::vector<float> &colors = {1.f, 0, 0}, int mode = 0);

    Handle Box(const std::array<float, 3> &pos,
               const std::array<float, 3> &extents,
               const std::vector<float> &color = {1.f, 0, 0});
    Handle Sphere(const std::array<float, 3> &center, float raidus,
                  const std::vector<float> &color = {1.f, 0, 0});
    Handle Spheres(const std::vector<float> &centers, std::vector<float> &radii,
                   const std::vector<float> &colors = {1.f, 0, 0, 1.f});
    Handle Cone(const std::array<float, 3> &center, float radius, float height,
                const std::vector<float> &color = {1.f, 0, 0});
    Handle Cylinder(const std::array<float, 3> &center, float radius,
                    float height,
                    const std::vector<float> &color = {1.f, 0, 0});
    Handle Arrow(const std::array<float, 3> &tail,
                 const std::array<float, 3> &head, float radius,
                 const std::vector<float> &color = {1.0f, 0, 0});
    Handle Mesh(const std::vector<float> &vertices,
                const std::vector<unsigned int> &indices,
                const std::vector<float> &colors = {1.f, 0, 0});

    /**
     * Plot a plane
     *
     * We first plot the plane with the center at the world origin, then
     * transfrom it to the target place with provided T
     *
     * @code
     * h = v.Plane(1, 1);
     * v.SetTransform(h);
     * @endcode
     * @param xlength length of the plane along x-axis
     * @param ylength length of the plane along y-axis
     * @param half_x_num_cells half number of cells along x-axis
     * @param half_y_num_cells half number of cells along y-axis
     * @param color color of the plane
     * @return Handle
     */
    Handle Plane(float xlength, float ylength, int half_x_num_cells = 8,
                 int half_y_num_cells = 8,
                 const std::vector<float> &color = {0.5, 0.5, 0.5});

    /**
     * Plane
     *
     * We plot the plane with the center at the world origin and the quaternion.
     *
     * @code
     * h = v.Plane(2, 2, 1, 1, (1, 1, 1), (0, 0, 0, 1))
     * @endcode
     * @param xlength length of the plane along x-axis
     * @param ylength length of the plane along y-axis
     * @param half_x_num_cells half number of cells along x-axis
     * @param half_y_num_cells half number of cells along y-axis
     * @param trans center of the plane
     * @param quat  Unit quaternion to rotate the plane with (x, y, z, w)
     * @param color color of the plane
     * @return Handle
     */
    Handle Plane(float xlength, float ylength, int half_x_num_cells,
                 int half_y_num_cells, const std::array<float, 3> &trans,
                 const std::array<float, 4> &quat,
                 const std::vector<float> &color = {0.5, 0.5, 0.5});

    /**
     * SetInterSectorMode
     *
     * Set Instersect Mode.
     * Use PickedPlane() to get picked position and norm .
     *
     * @code
     * v.SetInterSectMode(ViewObjectType_Point, true);
     * @endcode
     * @param mode ViewObjectType, ViewObjectType_None for intersect all.
     * @param hover when hover is true intersect every frame.
     */
    void SetIntersectMode(IntersectorMode mode, bool hover = false);

    /**
     * EnableGizmo
     *
     * disable default manipulator and add auxiliary points, lines and plane to
     * move, rotate and scale the model. Only one model enable at a time.
     *
     * @code
     * bool b = v.ShowGizmo(h, MOVE);
     * @endcode
     * @param h model`s handle.
     * @param gizmotype operation type, MOVE/ROTATE/SCALE/MOVE&ROTATE (1/2/3/4)
     * @return true if success
     */
    bool EnableGizmo(const Handle &h, int gizmotype);

    /**
     * SetGizmoType
     *
     * set Gizmo control type
     *
     * @code
     * bool b = v.SetGizmoType(1);
     * @endcode
     * @param gizmo operation type, MOVE/ROTATE/SCALE (1/2/3)
     * @param loctype location type, WORLD/LOCAL (1/2)  --- not implement
     * @return true if success
     */
    bool SetGizmoType(int gizmotype, int loctype = 2);

    /**
     * SetGizmoDrawMask
     *
     * set Gizmo to draw specific axis
     *
     * @code
     * bool b = v.SetGizmoDrawMask(1);
     * @endcode
     * @param optype operation type, MOVE/ROTATE/SCALE (1/2/3)
     * @param draw mask
     * MOVE : X/Y/Z/YZ/XZ/XY/ALL (1/2/4/8/16/32/63)
     * ROTATE : X/Y/Z/TRACKBALL/SCREEN/ALL (1/2/4/8/16/31)
     * SCALE : X/Y/Z/YZ/XZ/XY/ALL (1/2/4/8/16/32/63)
     * @return true if success
     */
    bool SetGizmoDrawMask(int gizmotype, int mask);

    /**
     * SetGizmoDisplayScale
     *
     * set Gizmo display scale ratio
     *
     * @code
     * bool b = v.SetGizmoScale(1);
     * @endcode
     * @param scale ratio, from 0.0 to MAX_FLOAT
     * default is 1.0
     * @return true if success
     */
    bool SetGizmoDisplayScale(float scale);

    /**
     * SetGizmoDetectionRange
     *
     * set Gizmo detection range
     *
     * @code
     * bool b = v.SetGizmoDetectionRange(1);
     * @endcode
     * @param detection range, from 0.0 to 1.0f
     * default is 0.1f
     * @return true if success
     */
    bool SetGizmoDetectionRange(float range);

    /**
     * DisableGizmo
     *
     * disable gizmo
     *
     * @code
     * bool b = v.DisableGizmo();
     * @endcode
     * @return true if success
     */
    bool DisableGizmo();

    /**
     * EnableShortcutKey
     *
     * enable/disable shortcut keys
     *
     * @code
     * bool b = v.EnableShortcutKey();
     * @endcode
     * @return current stat of Short keys
     */
    bool EnableShortcutKey();

    bool EnableTrackballManipulation(bool enable);

    bool SetColor(const Handle &nh, const std::vector<float> &color,
                  int color_channels = 0);
    bool GetColor(const Handle &nh, std::vector<float> &color) const;

    // // TODO: Line, Plane, Box, Arrow, Axes, Texture
    bool SetPosition(const Handle &nh, const std::array<float, 3> &pos);
    bool SetRotation(const Handle &nh, const std::array<float, 4> &quat);
    bool SetTransform(const Handle &nh, const std::array<float, 3> &pos,
                      const std::array<float, 4> &quat);

    bool GetPosition(const Handle &nh, std::array<float, 3> &pos);
    bool GetRotation(const Handle &nh, std::array<float, 4> &quat);
    bool GetTransform(const Handle &nh, std::array<float, 3> &pos,
                      std::array<float, 4> &quat);

private:
    Handle Clone(const Handle &nh, const osg::Matrix &m);
    Handle Load(const std::string &fname, const osg::Matrix &m);
    Handle Axes(const osg::Matrix &m, float axis_len, float axis_size);
    osgViewer::Viewer *GetOsgViewer();

private:
    std::shared_ptr<Vis3d> m_vis3d;
};
} // namespace Vis