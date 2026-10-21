"""Create portable, bevelled scene meshes for Unreal. Run with Blender --background --python."""
from pathlib import Path
import math
import sys
import bpy

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceAssets"
OUT.mkdir(parents=True, exist_ok=True)
ONLY = set(sys.argv[sys.argv.index("--")+1:]) if "--" in sys.argv else set()
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 0.01

def export(obj, name):
    if ONLY and name not in ONLY:
        bpy.data.objects.remove(obj, do_unlink=True)
        return
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.name = name
    bpy.ops.export_scene.fbx(filepath=str(OUT / (name + ".fbx")), use_selection=True,
        object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Y", axis_up="Z", bake_space_transform=False, use_mesh_modifiers=True,
        mesh_smooth_type="FACE", add_leaf_bones=False)
    bpy.data.objects.remove(obj, do_unlink=True)

def box(name, dims, bevel):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj=bpy.context.object
    obj.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    # Planar projection follows the grain of each physical piece.
    uv=obj.data.uv_layers.active
    for poly in obj.data.polygons:
        for li in poly.loop_indices:
            v=obj.data.vertices[obj.data.loops[li].vertex_index].co
            if abs(poly.normal.z)>.5: coords=(v.y/165+.5,v.x/250+.5)
            elif abs(poly.normal.y)>.5: coords=(v.z/165+.3,v.x/250+.5)
            else: coords=(v.y/165+.5,v.z/250+.3)
            uv.data[li].uv=coords
    mod=obj.modifiers.new("Real edge bevels","BEVEL");mod.width=bevel;mod.segments=4
    obj.modifiers.new("Weighted corner normals","WEIGHTED_NORMAL")
    export(obj,name)

box("TableTop",(250,132,8),.6)
box("TableLeg",(11,11,74),.3)
box("TableApron",(232,5.6,9),.15)
print("Exported table meshes to",OUT)
