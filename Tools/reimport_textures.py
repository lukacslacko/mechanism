"""Import bundled source maps with portable source paths and known texture settings."""
from pathlib import Path
import unreal as u

root=Path(__file__).resolve().parents[1]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cvar='Interchange.FeatureFlags.Import.Enable'
previous=u.SystemLibrary.get_console_variable_int_value(cvar)
u.SystemLibrary.execute_console_command(world,cvar+' 0')
try:
    for asset,source in [('WalnutColor','wood-color.jpg'),('WalnutNormal','wood-normal.jpg'),('WalnutRoughness','wood-roughness.jpg')]:
        source_path=root/'SourceAssets/Textures'/source
        assert source_path.is_file(),source_path
        task=u.AssetImportTask()
        for key,value in dict(filename=str(source_path),destination_path='/Game/MaterialHall/Assets',destination_name=asset,
                              automated=True,replace_existing=True,replace_existing_settings=True,save=True).items():
            task.set_editor_property(key,value)
        task.set_editor_property('factory',u.TextureFactory())
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture=u.load_asset('/Game/MaterialHall/Assets/'+asset)
        assert texture,asset
        texture.set_editor_property('srgb',asset=='WalnutColor')
        if asset=='WalnutNormal':
            texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
            texture.set_editor_property('flip_green_channel',True)
        texture.get_editor_property('asset_import_data').scripted_add_filename(str(source_path),0,'')
        assert u.EditorAssetLibrary.save_loaded_asset(texture)
        u.log('BUNDLED TEXTURE SOURCE IMPORTED: '+asset)
finally:
    u.SystemLibrary.execute_console_command(world,cvar+' '+str(previous))
