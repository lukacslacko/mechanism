"""Create the world-space UI material; keep its luminance stable in the sunlit hall."""
import unreal as u

destination = '/Game/MaterialHall/Materials/VRWidget'
material = u.load_asset(destination)
if not material:
    material = u.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/Widget3DPassThrough', destination)
    assert material, 'Could not copy the standard widget material'
    library = u.MaterialEditingLibrary
    source = library.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
    output = library.get_material_property_input_node_output_name(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
    inverse = library.create_material_expression(material, u.MaterialExpressionEyeAdaptationInverse, 700, 0)
    assert library.connect_material_expressions(source, output, inverse, '')
    assert library.connect_material_property(inverse, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    library.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log('VR UI material ready: ' + material.get_path_name())
