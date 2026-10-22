"""Original axial feedback spring. Blender --background --python."""
from pathlib import Path
import math
import bpy
import bmesh

root = Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = .01
curve = bpy.data.curves.new('FeedbackSpring', 'CURVE')
curve.dimensions = '3D'
curve.bevel_depth = .28
curve.bevel_resolution = 3
curve.use_fill_caps = True
spline = curve.splines.new('POLY')
count = 321
spline.points.add(count-1)
for i, point in enumerate(spline.points):
    t = i/(count-1)
    angle = t*math.tau*8
    point.co = (-10+20*t, 2.2*math.cos(angle), 2.2*math.sin(angle), 1)
obj = bpy.data.objects.new('LatchSpring', curve)
bpy.context.collection.objects.link(obj)
obj.select_set(True)
bpy.context.view_layer.objects.active = obj
bpy.ops.object.convert(target='MESH')
for poly in obj.data.polygons:
    poly.use_smooth = True
bm = bmesh.new()
bm.from_mesh(obj.data)
bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=.00001)
bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
assert all(edge.is_manifold for edge in bm.edges)
assert abs(bm.calc_volume()) > 0
bm.to_mesh(obj.data)
bm.free()
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(island_margin=.02)
bpy.ops.object.mode_set(mode='OBJECT')
out = root/'SourceAssets'/'AndGate'/'LatchSpring.fbx'
bpy.ops.export_scene.fbx(filepath=str(out), use_selection=True, object_types={'MESH'},
    apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL', axis_forward='-Y', axis_up='Z',
    mesh_smooth_type='FACE', add_leaf_bones=False)
print('LATCH SPRING READY', out)
