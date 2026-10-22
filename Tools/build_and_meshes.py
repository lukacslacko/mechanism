"""Original machined AND-gate parts, centimetres. Blender --background --python."""
from pathlib import Path
import math
import bpy
import bmesh

root = Path(__file__).resolve().parents[1]
out = root / 'SourceAssets' / 'AndGate'
out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = .01

def box(size, location=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj

def cylinder(radius, depth, location=(0,0,0), vertices=64):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    return bpy.context.object

def boolean(obj, other, operation):
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new(operation, 'BOOLEAN')
    mod.operation = operation
    mod.solver = 'EXACT'
    mod.object = other
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(other, do_unlink=True)

def finish(obj, name, bevel=.12):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    # Every exported part uses a known local origin, independent of its bounds.
    bpy.context.scene.cursor.location = (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    obj.name = name
    mod = obj.modifiers.new('Machined edge breaks', 'BEVEL')
    mod.width = bevel
    mod.segments = 3
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    for face in obj.data.polygons:
        face.use_smooth = abs(face.normal.z)<.5
    mod = obj.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
    mod.keep_sharp = True
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new(); bm.from_mesh(obj.data)
    assert all(e.is_manifold for e in bm.edges), name
    assert bm.calc_volume(signed=True)>0, name
    bm.free()
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
        axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE', add_leaf_bones=False)
    bpy.data.objects.remove(obj, do_unlink=True)

def link(length, name):
    obj = box((length,3.6,1.6))
    for x in (-length/2, length/2):
        boolean(obj, cylinder(3,1.6,(x,0,0)), 'UNION')
        boolean(obj, cylinder(1.05,5,(x,0,0)), 'DIFFERENCE')
    finish(obj, name)

a,h,s = 18,30.6,34.2
short = math.hypot(a,h)
d = math.hypot(2*a,s)
height = math.sqrt(short*short-d*d/4)
k = s/2 + 2*a*height/d-h
side = s*height/d
long = (side*side+k*k)/(2*k)
link(short,'AndInputLink')
link(long,'AndOutputLink')
for length,direction,name in ((65,-1,'AndInputSlider'),(64,1,'AndOutputSlider')):
    obj = box((length,3,3),(direction*length/2,0,0))
    boolean(obj,cylinder(3,3), 'UNION')
    boolean(obj,cylinder(1.05,6), 'DIFFERENCE')
    finish(obj,name)
obj = box((8,9,12))
boolean(obj,box((12,3.3,3.3)), 'DIFFERENCE')
finish(obj,'AndGuide',.22)
finish(box((240,104,2)),'AndBase',.5)
finish(box((1,1,1)),'AndBlock',.08)
finish(cylinder(1,10),'AndPin',.08)
obj = cylinder(1.8,.4)
boolean(obj,cylinder(1.05,3),'DIFFERENCE')
finish(obj,'AndWasher',.04)
obj = cylinder(1.65,.7,vertices=6)
boolean(obj,box((2.2,.25,.4),(0,0,.3)),'DIFFERENCE')
finish(obj,'AndBolt',.06)
finish(cylinder(4.3,3),'AndHandle',.25)
print('AND meshes exported. Link lengths:',short,long)
