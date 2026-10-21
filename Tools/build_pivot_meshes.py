"""Machined compass assembly, centimetres. Run with Blender --background --python.

The brass bore and steel collar bore are actual open passages, not textures.
"""
from pathlib import Path
import json
import math
import bpy
import bmesh
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceAssets"
REPORT = ROOT / "Saved/Validation/mesh-check.json"
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = .01
records = {}


def cube(dimensions):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.object
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def cylinder(radius, height):
    bpy.ops.mesh.primitive_cylinder_add(vertices=192, radius=radius, depth=height)
    return bpy.context.object


def bore(obj, radius, height):
    cutter = cylinder(radius, height + 4)
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new("Through bore", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def finish(obj, name, bevel, bore_radius=None):
    bpy.context.view_layer.objects.active = obj
    obj.name = name
    for poly in obj.data.polygons:
        poly.use_smooth = abs(poly.normal.z) < .5
    mod = obj.modifiers.new("Machined edge rounds", "BEVEL")
    mod.width = bevel
    mod.segments = 4
    bpy.ops.object.modifier_apply(modifier=mod.name)
    # UVs supply stable tangents for the metal's anisotropy.
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    mod = obj.modifiers.new("Weighted face normals", "WEIGHTED_NORMAL")
    mod.keep_sharp = True
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    assert all(edge.is_manifold for edge in bm.edges), name
    volume = bm.calc_volume(signed=True)
    assert volume > 0, (name, volume)
    bm.free()
    record = {"watertight": True, "volume_cm3": volume,
              "dimensions_cm": list(obj.dimensions), "edge_round_cm": bevel}
    if bore_radius is not None:
        # Sample the pin's entire circular footprint through the brass bore.
        # A filled cap or a hidden surface through the bore must fail this test.
        for radius in [0, 1.49]:
            for angle in range(0, 360, 5):
                xy = Vector((radius*math.cos(math.radians(angle)),
                             radius*math.sin(math.radians(angle)), 20))
                assert not obj.ray_cast(xy, Vector((0, 0, -1)), distance=40)[0], (name, xy)
        assert obj.ray_cast(Vector((bore_radius+.3, 0, 20)), Vector((0, 0, -1)), distance=40)[0]
        hit, position, _, _ = obj.ray_cast(Vector((0, 0, 0)), Vector((1, 0, 0)), distance=10)
        assert hit and abs(position.x-bore_radius) < .003, (name, position)
        record.update(bore_radius_cm=bore_radius, open_through_bore=True,
                      sampled_clear_pin_radius_cm=1.49)
    records[name] = record
    bpy.ops.export_scene.fbx(filepath=str(OUT / (name+".fbx")), use_selection=True,
        object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Y", axis_up="Z", bake_space_transform=False,
        use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False)
    bpy.data.objects.remove(obj, do_unlink=True)


finish(cube((100, 100, 1.6)), "SteelPlate", .18)
finish(cylinder(1.5, 9.8), "PivotPin", .10)
collar = cylinder(3.2, .8)
bore(collar, 1.5, .8)
finish(collar, "PivotCollar", .06, 1.5)
pointer = cube((80, 7.4, 7.4))
bore(pointer, 1.7, 7.4)
finish(pointer, "BrassPointer", .12, 1.7)
REPORT.parent.mkdir(parents=True, exist_ok=True)
REPORT.write_text(json.dumps({"passed": True, "meshes": records}, indent=2))
print("PIVOT MESHES EXPORTED AND VERIFIED", REPORT)
