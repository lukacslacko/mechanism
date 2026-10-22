"""Import the generated gate parts and create original metal finishes."""
from pathlib import Path
import unreal as u
root = Path(__file__).resolve().parents[1]
destination = '/Game/MaterialHall/AndGate'
tasks = []
for path in sorted((root/'SourceAssets'/'AndGate').glob('*.fbx')):
    task = u.AssetImportTask()
    for key,value in dict(filename=str(path), destination_path=destination, destination_name=path.stem,
                          automated=True, replace_existing=True, save=True).items():
        task.set_editor_property(key,value)
    options = u.FbxImportUI()
    for key,value in dict(import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,
                         automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH).items():
        options.set_editor_property(key,value)
    options.static_mesh_import_data.set_editor_property('combine_meshes',True)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
    options.static_mesh_import_data.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.set_editor_property('options',options)
    tasks.append(task)
assert len(tasks)>=11, len(tasks)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
subsystem = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.StaticMeshEditorSubsystem()
for task in tasks:
    mesh = u.load_asset(destination+'/'+task.destination_name)
    assert mesh
    settings = subsystem.get_lod_build_settings(mesh,0)
    settings.set_editor_property('distance_field_resolution_scale',2)
    subsystem.set_lod_build_settings(mesh,0,settings)
    u.EditorAssetLibrary.save_loaded_asset(mesh)

def material(name,color,metallic,roughness):
    mat = u.load_asset(destination+'/'+name)
    if mat: return
    mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name,destination,u.Material,u.MaterialFactoryNew())
    ed = u.MaterialEditingLibrary
    rgb = ed.create_material_expression(mat,u.MaterialExpressionVectorParameter,-400,0)
    rgb.set_editor_property('parameter_name','Color')
    rgb.set_editor_property('default_value',u.LinearColor(*color,1))
    ed.connect_material_property(rgb,'',u.MaterialProperty.MP_BASE_COLOR)
    for index,(param,value,prop) in enumerate((('Metallic',metallic,u.MaterialProperty.MP_METALLIC),('Roughness',roughness,u.MaterialProperty.MP_ROUGHNESS))):
        node = ed.create_material_expression(mat,u.MaterialExpressionScalarParameter,-400,180+index*120)
        node.set_editor_property('parameter_name',param)
        node.set_editor_property('default_value',value)
        ed.connect_material_property(node,'',prop)
    ed.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)

material('GateBase',(0.025,0.045,0.055),.65,.32)
material('GateSteel',(.48,.55,.58),.9,.22)
material('GateBrass',(.62,.36,.09),.85,.25)
material('GateInputA',(.025,.38,.48),.5,.25)
material('GateInputB',(.75,.22,.055),.5,.28)
material('GateOutput',(.11,.45,.21),.5,.25)
material('GateIvory',(.78,.75,.63),.15,.45)
u.log('AND GATE ASSETS READY')
