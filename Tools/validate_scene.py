"""Validate imported assembly bounds and support relationships in the saved level."""
from pathlib import Path
import json, math
import unreal as u
root='/Game/MaterialHall'
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(root+'/WhiteHall')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
meshes={}
for a in actors:
    if isinstance(a,u.StaticMeshActor) and a.static_mesh_component.static_mesh:
        name=a.static_mesh_component.static_mesh.get_name()
        if name in ['TableTop','SteelPlate','PivotPin','PivotCollar','BrassPointer']:
            assert name not in meshes, name
            meshes[name]=a
        assert name not in ['MetalRod','SquareRod'], 'Old loose rod remains'
assert len(meshes)==5,meshes
expected={'SteelPlate':(50,50,.8),'PivotPin':(1.5,1.5,4.9),'PivotCollar':(3.2,3.2,.4),'BrassPointer':(40,3.7,3.7)}
for name,extent in expected.items():
    actor=meshes[name]
    bounds=actor.static_mesh_component.static_mesh.get_bounds().box_extent
    assert all(abs(a-b)<.01 for a,b in zip((bounds.x,bounds.y,bounds.z),extent)),(name,bounds)
    rotation=actor.get_actor_rotation()
    assert abs(rotation.pitch)<.0001 and abs(rotation.roll)<.0001
    pos=actor.get_actor_location()
    assert abs(pos.x)<.0001 and abs(pos.y)<.0001,(name,pos)
    assert actor.static_mesh_component.get_material(0).get_name()==('Brass' if name=='BrassPointer' else 'BrushedSteel')

def limits(name):
    actor=meshes[name]
    low,high=actor.static_mesh_component.get_local_bounds()
    p=actor.get_actor_location()
    return p.z+low.z,p.z+high.z

table=limits('TableTop'); plate=limits('SteelPlate'); collar=limits('PivotCollar'); pin=limits('PivotPin'); bar=limits('BrassPointer')
errors=[plate[0]-table[1],collar[0]-plate[1],pin[0]-plate[1],bar[0]-collar[1]]
assert all(abs(e)<.001 for e in errors),errors
assert pin[1]>bar[1] and bar[0]>plate[1]
assert meshes['BrassPointer'].static_mesh_component.mobility==u.ComponentMobility.MOVABLE
assert len([a for a in actors if isinstance(a,u.RectLight)])==3
assert len([a for a in actors if isinstance(a,u.CameraActor)])==1
report={'passed':True,'actor_count':len(actors),'table_top_z_cm':table[1],
        'steel_plate_z_cm':plate,'collar_z_cm':collar,'pivot_pin_z_cm':pin,'brass_pointer_z_cm':bar,
        'support_errors_cm':errors,'radial_pin_bore_clearance_cm':.2,
        'pin_protrusion_cm':pin[1]-bar[1],'pointer_plate_gap_cm':bar[0]-plate[1]}
report_path=Path(__file__).resolve().parents[1]/'Saved/Validation/scene-check.json'
report_path.parent.mkdir(parents=True,exist_ok=True)
report_path.write_text(json.dumps(report,indent=2))
u.log('UNREAL PIVOT SCENE VALIDATION PASSED')
