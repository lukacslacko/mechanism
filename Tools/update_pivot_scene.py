"""Install the compass assembly in the existing WhiteHall level, without opening UI."""
from pathlib import Path
import runpy
import unreal as u

root=Path(__file__).resolve().parents[1]
assetroot='/Game/MaterialHall'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level(assetroot+'/WhiteHall')
names=['SteelPlate','PivotPin','PivotCollar','BrassPointer']
tasks=[]
for name in names:
    task=u.AssetImportTask()
    for k,v in dict(filename=str(root/'SourceAssets'/(name+'.fbx')),destination_path=assetroot+'/Assets',destination_name=name,automated=True,replace_existing=True,save=True).items():
        task.set_editor_property(k,v)
    options=u.FbxImportUI()
    for k,v in dict(import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH).items():
        options.set_editor_property(k,v)
    options.static_mesh_import_data.set_editor_property('combine_meshes',True)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
    options.static_mesh_import_data.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.set_editor_property('options',options)
    tasks.append(task)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
sub=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.StaticMeshEditorSubsystem()
meshes={}
for name in names:
    mesh=u.load_asset(assetroot+'/Assets/'+name)
    assert mesh, name
    settings=sub.get_lod_build_settings(mesh,0)
    settings.set_editor_property('distance_field_resolution_scale',4 if name!='SteelPlate' else 2)
    settings.set_editor_property('max_lumen_mesh_cards',32)
    sub.set_lod_build_settings(mesh,0,settings)
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    meshes[name]=mesh
actors=editor.get_all_level_actors()
top=next(a for a in actors if a.get_actor_label()=='Walnut table top').static_mesh_component
_,hi=top.get_local_bounds()
table_z=u.MathLibrary.transform_location(top.get_world_transform(),u.Vector(0,0,hi.z)).z
for actor in actors:
    if isinstance(actor,u.StaticMeshActor):
        mesh=actor.static_mesh_component.static_mesh
        if mesh and mesh.get_name() in names+['MetalRod','SquareRod']:
            assert editor.destroy_actor(actor)
# Elevations are derived from actual imported bounds, not guessed mesh origins.
plate_height=meshes['SteelPlate'].get_bounds().box_extent.z*2
plate_top=table_z+plate_height
collar_height=meshes['PivotCollar'].get_bounds().box_extent.z*2
locations={
    'SteelPlate':table_z+plate_height/2,
    'PivotPin':plate_top+meshes['PivotPin'].get_bounds().box_extent.z,
    'PivotCollar':plate_top+collar_height/2,
    'BrassPointer':plate_top+collar_height+meshes['BrassPointer'].get_bounds().box_extent.z,
}
labels={'SteelPlate':'Square steel base plate','PivotPin':'Central steel pivot','PivotCollar':'Steel support collar','BrassPointer':'Rotating brass pointer'}
for name in names:
    actor=editor.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,locations[name]))
    actor.set_actor_label(labels[name])
    actor.set_editor_property('tags',[u.Name(name)])
    comp=actor.static_mesh_component
    comp.set_static_mesh(meshes[name])
    comp.set_material(0,u.load_asset(assetroot+'/Materials/'+('Brass' if name=='BrassPointer' else 'BrushedSteel')))
    if name=='BrassPointer':
        comp.set_mobility(u.ComponentMobility.MOVABLE)
        actor.set_actor_rotation(u.Rotator(yaw=25),False)
    comp.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
assert levels.save_current_level()
runpy.run_path(str(root/'Tools/validate_scene.py'))
u.log('COMPASS ASSEMBLY SAVED AND VALIDATED')
