"""Run INSIDE Unreal Editor after compiling AshenSpiralEditor.
Imports the bundled original art, creates the material and a real .umap.
This script does not require external Python packages or download any assets.
"""
from pathlib import Path
import hashlib
import json
import os
import traceback
import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ART = ROOT / "SourceArt"
MARKER = ROOT / "Saved" / "AshenBootstrap.json"
FORCE = os.environ.get("ASHEN_FORCE_IMPORT") == "1"
WEAPON_ART_REVISION = 10
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


COLLISION_MESHES = {"SM_ArchiveShelf", "SM_ArchiveShelfBroken", "SM_ArchiveDesk", "SM_ArchiveDeskBroken", "SM_Pillar", "SM_Arch", "SM_Shelf", "SM_Obelisk", "SM_Chest", "SM_Brazier", "SM_Skull", "SM_Tree", "SM_Rock", "SM_Crystal", "SM_CaveRock", "SM_CrystalCluster", "SM_LootCrystal", "SM_EmberCask", "SM_CaveVault", "SM_BarrelVault", "SM_Stalagmite", "SM_Stalactite", "SM_BasaltColumn", "SM_RootLog", "SM_Lantern"}


def precise_collision(asset, name):
    if name not in COLLISION_MESHES:
        return
    body = asset.get_editor_property("body_setup")
    require(body is not None, name + " has no collision body setup")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    body.set_editor_property("double_sided_geometry", True)
    require(body.get_editor_property("collision_trace_flag") == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
            name + " did not accept mesh-matching collision")
    assets.save_loaded_asset(asset, only_if_is_dirty=False)


def import_asset(path, destination, kind):
    package = destination + "/" + path.stem
    asset = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    # Setup runs after a source hash change; refresh the complete revised mesh kit.
    replace = FORCE or kind == "mesh"
    if asset is not None and not replace:
        if kind == "mesh": precise_collision(asset, path.stem)
        return asset
    if asset is not None and kind == "mesh":
        # 0.24.2: a mesh is imported clean rather than over the old one. Re-importing the multi-part meshes (weapons, the sniper, the dummy) over an existing asset failed
        # with "nothing to import" once their extra sections had been given materials, and the importer's errors failed the whole Setup even though the script went on.
        asset = None
        if assets.delete_asset(package):
            replace = False
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", path.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", replace)
    task.set_editor_property("save", True)
    if kind == "mesh":
        options = unreal.FbxImportUI()
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("import_animations", False)
        data = options.get_editor_property("static_mesh_import_data")
        for name, value in {
            "combine_meshes": True,
            "auto_generate_collision": True,
            "generate_lightmap_u_vs": False,
            "convert_scene": False,
            "convert_scene_unit": False,
            "import_uniform_scale": 1.0,
            "build_nanite": False,
        }.items():
            data.set_editor_property(name, value)
        data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task.set_editor_property("options", options)
        # Explicit factory avoids relying on machine-specific Interchange defaults.
        task.set_editor_property("factory", unreal.FbxFactory())
    tools.import_asset_tasks([task])
    asset = unreal.load_asset(package)
    require(asset is not None, "Import did not create {}. Source: {}. Check Bootstrap.log.".format(package, path))
    if kind == "mesh":
        require(isinstance(asset, unreal.StaticMesh), package + " is not a static mesh")
        precise_collision(asset, path.stem)
    return asset


def create_material():
    package = "/Game/Art/Materials/M_Surface"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_Surface", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create the master material")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)

    def node(cls, x, y, **properties):
        obj = edit.create_material_expression(material, cls, x, y)
        require(obj is not None, "Could not create material expression " + cls.__name__)
        for key, value in properties.items():
            obj.set_editor_property(key, value)
        return obj

    def wire(a, output, b, input_name):
        require(edit.connect_material_expressions(a, output, b, input_name), "Material connection failed: " + input_name)

    def out(a, output, prop):
        require(edit.connect_material_property(a, output, prop), "Material output connection failed: " + str(prop))

    tint = node(unreal.MaterialExpressionVectorParameter, -850, -240,
                parameter_name="Tint", default_value=unreal.LinearColor(0.4, 0.35, 0.28, 1))
    stone = node(unreal.MaterialExpressionTextureSampleParameter2D, -850, 10,
                 parameter_name="AlbedoMap", texture=unreal.load_asset("/Game/Art/Textures/T_Stone"))
    normal = node(unreal.MaterialExpressionTextureSampleParameter2D, -850, 270,
                  parameter_name="NormalMap", texture=unreal.load_asset("/Game/Art/Textures/T_Normal"),
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    roughmap = node(unreal.MaterialExpressionTextureSampleParameter2D, -850, 540,
                    parameter_name="RoughnessMap", texture=unreal.load_asset("/Game/Art/Textures/T_Roughness"),
                    sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    metallic = node(unreal.MaterialExpressionScalarParameter, -390, 600, parameter_name="Metallic", default_value=0.0)
    roughness = node(unreal.MaterialExpressionScalarParameter, -600, 810, parameter_name="Roughness", default_value=0.75)
    emission = node(unreal.MaterialExpressionScalarParameter, -600, -440, parameter_name="Emission", default_value=0.0)
    color = node(unreal.MaterialExpressionMultiply, -360, -150)
    glow = node(unreal.MaterialExpressionMultiply, -360, -370)
    rough = node(unreal.MaterialExpressionMultiply, -300, 440)
    detail = node(unreal.MaterialExpressionScalarParameter, -1000, 800, parameter_name="SurfaceDetail", default_value=1.0)
    white = node(unreal.MaterialExpressionConstant3Vector, -1000, -50, constant=unreal.LinearColor(1, 1, 1, 1))
    flat = node(unreal.MaterialExpressionConstant3Vector, -1000, 420, constant=unreal.LinearColor(0, 0, 1, 1))
    surface = node(unreal.MaterialExpressionLinearInterpolate, -590, 20)
    normal_mix = node(unreal.MaterialExpressionLinearInterpolate, -590, 300)
    rough_mix = node(unreal.MaterialExpressionLinearInterpolate, -590, 550)
    unit_rough = node(unreal.MaterialExpressionConstant, -850, 690, r=1.0)
    wire(white, "", surface, "A"); wire(stone, "RGB", surface, "B"); wire(detail, "", surface, "Alpha")
    wire(flat, "", normal_mix, "A"); wire(normal, "RGB", normal_mix, "B"); wire(detail, "", normal_mix, "Alpha")
    wire(unit_rough, "", rough_mix, "A"); wire(roughmap, "R", rough_mix, "B"); wire(detail, "", rough_mix, "Alpha")
    wire(tint, "", color, "A"); wire(surface, "", color, "B")
    wire(tint, "", glow, "A"); wire(emission, "", glow, "B")
    wire(rough_mix, "", rough, "A"); wire(roughness, "", rough, "B")
    out(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    out(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    out(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    out(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    out(normal_mix, "", unreal.MaterialProperty.MP_NORMAL)
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, True)  # 0.24.2 Phase F: the Mannequin hero's dash afterimages
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_Surface")
    return material


def create_glow_material():
    """0.24.2 aesthetics: M_Glow, the light of the effects (swing arcs, slashes, flashes, glints, dash afterimages). Unlit and additive, so an effect fading its Emission fades out
    altogether (on the lit, opaque M_Surface a fading glow left a solid grey shape, which is why the swing arc used to be hidden instead of faded). Same Tint and Emission
    parameters as M_Surface, so the effects drive either one; Emission is scaled by .35, since an unlit additive surface needs far less than a glow on top of a lit one."""
    package = "/Game/Art/Materials/M_Glow"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_Glow", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_Glow")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    tint = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    emission = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 100)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 1.0)
    scale = edit.create_material_expression(material, unreal.MaterialExpressionConstant, -600, 250)
    scale.set_editor_property("r", 0.35)
    strength = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 150)
    require(edit.connect_material_expressions(emission, "", strength, "A") and edit.connect_material_expressions(scale, "", strength, "B"), "M_Glow strength")
    glow = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, 0)
    require(edit.connect_material_expressions(tint, "", glow, "A") and edit.connect_material_expressions(strength, "", glow, "B"), "M_Glow colour")
    require(edit.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "M_Glow output")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, True)  # the Mannequin's dash afterimages
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_Glow")
    return material


def create_glow_band_material():
    """0.24.2 aesthetics: M_GlowBand, M_Glow for the flat bands of light - the ring flashes and shockwaves (SM_Ring), slashes (SM_Slash) and the swing arc (SM_SwingArc). With
    M_Glow those drew as solid cream cut-outs (a flat colour at high emission saturates to white); this one lights the band with a profile across it - soft at both edges
    (Edge 0), or brightest at the outer edge, the reach of a swing (Edge 1) - streaks that run round it, and a white-hot line where it is brightest. The meshes' UVs are their
    footprint (u, v) = (x, y) / 100, which the importer stores with v flipped, so the material measures the radius and angle from (u, 1 - v). Inner and Outer are the band's
    radii in those units (SM_Ring and SM_Slash .47-.53, SM_SwingArc .425-.5). Same Tint and Emission (x .35) as M_Glow."""
    package = "/Game/Art/Materials/M_GlowBand"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_GlowBand", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_GlowBand")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    sources = {"UV": edit.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -700, -300),
               "T": edit.create_material_expression(material, unreal.MaterialExpressionTime, -700, -200)}
    for row, (name, value) in enumerate((("Emission", 1.0), ("Inner", .47), ("Outer", .53), ("Edge", 0.0), ("Streaks", .5))):
        param = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, -100 + row * 100)
        param.set_editor_property("parameter_name", name)
        param.set_editor_property("default_value", value)
        sources[name] = param
    tint = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, 450)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    sources["Tint"] = tint
    code = ("float2 p=float2(UV.x,1-UV.y);float r=length(p);float t=saturate((r-Inner)/max(0.001,Outer-Inner));"
            "float band=lerp(sin(3.14159*t),pow(t,1.6)*saturate((1-t)*12),Edge);"
            "float a=atan2(p.y,p.x);float s=(0.5+0.5*sin(a*41+T*7+2.5*sin(a*9+T*2)))*(0.6+0.4*sin(a*17-T*5));"
            "float k=Emission*0.35;"
            "return Tint.rgb*k*band*lerp(1,0.35+0.9*s,Streaks)+k*0.45*pow(saturate(band),6);")
    custom = edit.create_material_expression(material, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "Band of light")
    custom.set_editor_property("code", code)
    inputs = []
    for name in sources:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", name)
        inputs.append(item)
    custom.set_editor_property("inputs", inputs)
    for name, source in sources.items():
        require(edit.connect_material_expressions(source, "", custom, name), "M_GlowBand input " + name)
    require(edit.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "M_GlowBand output")
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_GlowBand")
    return material


def create_portal_material():
    """0.24.2 aesthetics: M_Portal, the face of a portal - a slow vortex of spiral arms turning against a fainter counter-swirl, a white-hot core and a bright rim, in the
    portal's colour; a sealed portal is the same vortex, dark and slow (the game sets Tint, Emission and Speed). Unlit. The lens is the engine's sphere (radius 50) pressed flat
    along x, so the pattern is laid out on its local y and z."""
    package = "/Game/Art/Materials/M_Portal"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_Portal", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_Portal")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    sources = {"LP": edit.create_material_expression(material, unreal.MaterialExpressionLocalPosition, -700, -300),
               "T": edit.create_material_expression(material, unreal.MaterialExpressionTime, -700, -200)}
    for row, (name, value) in enumerate((("Emission", 1.0), ("Speed", 1.0), ("Arms", 3.0), ("Twist", 6.0))):
        param = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, -100 + row * 100)
        param.set_editor_property("parameter_name", name)
        param.set_editor_property("default_value", value)
        sources[name] = param
    tint = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, 350)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, .5, .2, 1))
    sources["Tint"] = tint
    code = ("float2 p=LP.yz/50.0;float r=saturate(length(p));float a=atan2(p.y,p.x);float t=T*Speed;"
            "float s1=0.5+0.5*sin(a*Arms+r*Twist-t*3.0);float s2=0.5+0.5*sin(a*(Arms+2.0)-r*Twist*1.7+t*2.2);"
            "float arms=s1*s1*(0.45+0.55*s2);float core=exp(-r*r*18.0);float rim=smoothstep(0.82,1.0,r);"
            "float lum=dot(Tint.rgb,float3(0.3,0.59,0.11));"  # the core runs hot in the portal's own colour: a dark, sealed portal keeps a dark heart
            "float3 c=Tint.rgb*(0.06+1.5*arms)*(1.0-0.35*r)+(Tint.rgb*1.2+lum*0.5)*core+Tint.rgb*rim*0.6;"
            "return c*Emission*0.3;")
    custom = edit.create_material_expression(material, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "Portal vortex")
    custom.set_editor_property("code", code)
    inputs = []
    for name in sources:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", name)
        inputs.append(item)
    custom.set_editor_property("inputs", inputs)
    for name, source in sources.items():
        require(edit.connect_material_expressions(source, "", custom, name), "M_Portal input " + name)
    require(edit.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "M_Portal output")
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_Portal")
    return material


def create_flame_material():
    """0.24.2 aesthetics: M_Flame, a flame on the engine's sphere (radius 50, the game stretches it upward): white-hot at its foot, turning to the Tint toward a thinning
    tip, licks running up it, soft at the silhouette. Unlit and additive, so it is light. The hub's station flames were glowing spheres that bloomed into white eggs."""
    package = "/Game/Art/Materials/M_Flame"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_Flame", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_Flame")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    sources = {"LP": edit.create_material_expression(material, unreal.MaterialExpressionLocalPosition, -700, -400),
               "T": edit.create_material_expression(material, unreal.MaterialExpressionTime, -700, -300),
               "N": edit.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, -700, -200),
               "V": edit.create_material_expression(material, unreal.MaterialExpressionCameraVectorWS, -700, -100)}
    for row, (name, value) in enumerate((("Emission", 1.0), ("Flicker", 1.0))):
        param = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, row * 100)
        param.set_editor_property("parameter_name", name)
        param.set_editor_property("default_value", value)
        sources[name] = param
    tint = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, .45, .08, 1))
    sources["Tint"] = tint
    code = ("float h=saturate(LP.z/100.0+0.5);float facing=saturate(dot(normalize(N),normalize(V)));float body=facing*facing;"
            "float a=atan2(LP.y,LP.x);float t=T*Flicker;"
            "float lick=0.6+0.4*sin(a*5.0+h*9.0-t*7.0)*sin(a*3.0-h*6.0+t*5.0);"
            "float taper=saturate(1.15-h*h*1.3);"
            "float i=body*taper*(0.55+0.45*lick*h)+pow(body,6.0)*(1.0-h)*0.8;"
            "float3 hot=lerp(float3(1.0,0.95,0.8),Tint.rgb,saturate(h*1.4));"
            "return hot*i*Emission*0.25;")
    custom = edit.create_material_expression(material, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "Flame")
    custom.set_editor_property("code", code)
    inputs = []
    for name in sources:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", name)
        inputs.append(item)
    custom.set_editor_property("inputs", inputs)
    for name, source in sources.items():
        require(edit.connect_material_expressions(source, "", custom, name), "M_Flame input " + name)
    require(edit.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "M_Flame output")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)  # the hub's flames are one instanced batch
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_Flame")
    return material


def main():
    MARKER.parent.mkdir(parents=True, exist_ok=True)
    MARKER.unlink(missing_ok=True)
    require(ART.is_dir(), "Missing SourceArt folder. Extract the complete ZIP first.")
    require(unreal.load_class(None, "/Script/AshenSpiral.ASGameMode") is not None,
            "Native AshenSpiral module missing. Run the Editor build before this script.")
    imported = []
    for folder in ("/Game/Art/Meshes", "/Game/Art/Textures", "/Game/Art/Materials", "/Game/Audio", "/Game/Maps"):
        assets.make_directory(folder)
    for file in sorted((ART / "Textures").glob("*.png")):
        texture = import_asset(file, "/Game/Art/Textures", "texture")
        if file.stem.endswith("_Normal"):
            texture.set_editor_property("srgb", False)
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif file.stem.endswith("_Roughness"):
            texture.set_editor_property("srgb", False)
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        assets.save_loaded_asset(texture, False)
        imported.append(texture.get_path_name())
    material = create_material()
    glow = create_glow_material()
    band = create_glow_band_material()
    portal = create_portal_material()
    flame = create_flame_material()
    manifest_path = ART / "mesh_bounds.json"
    expected = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    for file in sorted((ART / "Meshes").glob("*.obj")):
        mesh = import_asset(file, "/Game/Art/Meshes", "mesh")
        # 0.24.2: every slot - a mesh's parts (a weapon's grip, blade, guard...) import as separate sections, and a slot left empty draws the engine's grey grid
        for slot in range(max(1, len(mesh.get_editor_property("static_materials")))):
            mesh.set_material(slot, material)
        # Verify centimeters and Z-up instead of silently shipping rotated/scaled content.
        if file.stem in expected:
            bounds = mesh.get_bounding_box()
            actual = [bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z]
            target = expected[file.stem]["extent"]
            require(all(abs(a-b) <= max(2.0, abs(b)*0.03) for a,b in zip(actual,target)),
                    "Mesh scale/axis mismatch for {}: imported {}, expected {} cm. Keep OBJ conversion disabled.".format(file.stem, actual, target))
        require(assets.save_loaded_asset(mesh, False), "Could not save " + file.stem)
        imported.append(mesh.get_path_name())
    for file in sorted((ART / "Audio").glob("*.wav")):
        sound = import_asset(file, "/Game/Audio", "audio")
        imported.append(sound.get_path_name())
    map_path = "/Game/Maps/AshenSpiral"
    if not assets.does_asset_exist(map_path):
        world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        require(world is not None, "Could not create a blank world")
        require(unreal.EditorLoadingAndSavingUtils.save_map(world, map_path), "Could not save AshenSpiral.umap")
    # The GameMode builds the complete hub/rooms when Play starts. A blank edit viewport is expected.
    require((ROOT / "Content/Maps/AshenSpiral.umap").is_file(), "The .umap did not reach disk")
    assets.save_directory("/Game/Art", False, True)
    assets.save_directory("/Game/Audio", False, True)
    # Every mesh, texture and sound in SourceArt is imported, so the expected count follows the folders (82 meshes, 12 textures and 12 sounds at 0.24, plus any enemy cue
    # files delivered into SourceArt/Audio).
    expected_count = len(list((ART / "Meshes").glob("*.obj"))) + len(list((ART / "Textures").glob("*.png"))) + len(list((ART / "Audio").glob("*.wav")))
    require(len(imported) == expected_count, "Incomplete art import: imported {} of {} source files".format(len(imported), expected_count))
    report = {"success": True, "source_version": "0.24.1", "weapon_art_revision": WEAPON_ART_REVISION, "engine": unreal.SystemLibrary.get_engine_version(),
              "asset_count": len(imported) + 6, "assets": imported + [material.get_path_name(), glow.get_path_name(), band.get_path_name(), portal.get_path_name(), flame.get_path_name(), map_path],
              "bootstrap_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    MARKER.write_text(json.dumps(report, indent=2))
    unreal.log("ASHEN BOOTSTRAP COMPLETE: {} assets/map entries. Press Play in the AshenSpiral map.".format(report["asset_count"]))


try:
    main()
except Exception:
    MARKER.unlink(missing_ok=True)
    unreal.log_error("ASHEN BOOTSTRAP FAILED\n" + traceback.format_exc())
    raise
