"""
Génère une table de blackjack : FBX + normal map + texture Base Color (bois / feutre).

Géométrie en CENTIMÈTRES, export FBX global_scale=1.0 (1 uu Unreal = 1 cm).

Sortie :
  - BlackjackTable.fbx
  - BlackjackTable_Normal.png
  - BlackjackTable_BaseColor.png

Usage :
  blender --background --python generate_blackjack_table.py
"""

import bpy
import os
import math

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(SCRIPT_DIR, "export")
os.makedirs(OUT_DIR, exist_ok=True)

FBX_PATH = os.path.join(OUT_DIR, "BlackjackTable.fbx")
NORMAL_PATH = os.path.join(OUT_DIR, "BlackjackTable_Normal.png")
BASECOLOR_PATH = os.path.join(OUT_DIR, "BlackjackTable_BaseColor.png")

W, D = 220.0, 110.0
H = 76.0
FELT_T = 4.5
RAIL_T = 5.5
LEG_R = 5.5
LEG_HEIGHT = H - FELT_T - RAIL_T * 0.5

BAKE_SIZE = 2048
CAGE_EXTRUSION = 2.0
BEVEL_WIDTH_CM = 1.2

MAT_WOOD_NAME = "TH_BJ_Wood"
MAT_FELT_NAME = "TH_BJ_Felt"
# Couleurs « casino » (modifiables) — bake en sRGB
WOOD_RGBA = (0.32, 0.18, 0.10, 1.0)
FELT_RGBA = (0.04, 0.38, 0.14, 1.0)


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def apply_scale(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)


def make_principled_mat(name, rgba):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = rgba
        bsdf.inputs["Roughness"].default_value = 0.65
    return mat


def assign_mat(obj, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat)


def create_table_mesh():
    wood = make_principled_mat(MAT_WOOD_NAME, WOOD_RGBA)
    felt_m = make_principled_mat(MAT_FELT_NAME, FELT_RGBA)

    m = 8.0
    hw = W * 0.5 - LEG_R - m
    hd = D * 0.5 - LEG_R - m
    for x, y in [(hw, hd), (-hw, hd), (-hw, -hd), (hw, -hd)]:
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=20,
            radius=LEG_R,
            depth=LEG_HEIGHT,
            location=(x, y, LEG_HEIGHT * 0.5),
        )
        assign_mat(bpy.context.active_object, wood)

    z_plate = LEG_HEIGHT + RAIL_T * 0.5
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, z_plate))
    plate = bpy.context.active_object
    plate.name = "Plateau"
    plate.scale = (W * 0.98, D * 0.98, RAIL_T)
    assign_mat(plate, wood)

    z_felt = LEG_HEIGHT + RAIL_T + FELT_T * 0.5
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, z_felt))
    felt_ob = bpy.context.active_object
    felt_ob.name = "Feutre"
    felt_ob.scale = (W * 0.94, D * 0.88, FELT_T)
    assign_mat(felt_ob, felt_m)

    outer_w, outer_d = W + 6.0, D + 6.0
    band_t = 4.5
    z_band = z_felt + FELT_T * 0.5 + band_t * 0.5

    def band(sx, sy, sz, lx, ly, lz):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(lx, ly, lz))
        o = bpy.context.active_object
        o.scale = (sx, sy, sz)
        assign_mat(o, wood)
        return o

    band(outer_w, band_t, band_t, 0, 0.5 * (D + band_t), z_band)
    band(outer_w, band_t, band_t, 0, -0.5 * (D + band_t), z_band)
    band(band_t, D, band_t, 0.5 * (W + band_t), 0, z_band)
    band(band_t, D, band_t, -0.5 * (W + band_t), 0, z_band)

    bpy.ops.object.select_all(action="DESELECT")
    objs = [ob for ob in bpy.context.scene.objects if ob.type == "MESH"]
    for ob in objs:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    joined = bpy.context.active_object
    joined.name = "BlackjackTable_Low"
    apply_scale(joined)
    bpy.ops.object.select_all(action="DESELECT")
    joined.select_set(True)
    bpy.context.view_layer.objects.active = joined
    bpy.ops.object.shade_smooth()
    return joined


def unwrap_mesh(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=45.0, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")


def store_vertex_colors_from_material_slots(obj):
    """Copie bois / feutre des material_index vers l’attribut Col (pour bake albedo)."""
    mesh = obj.data
    felt_idx = mesh.materials.find(MAT_FELT_NAME)
    if felt_idx < 0:
        felt_idx = 1 if len(mesh.materials) > 1 else 0

    if "Col" in mesh.color_attributes:
        mesh.color_attributes.remove(mesh.color_attributes["Col"])
    ca = mesh.color_attributes.new(name="Col", type="FLOAT_COLOR", domain="CORNER")

    for poly in mesh.polygons:
        col = FELT_RGBA if poly.material_index == felt_idx else WOOD_RGBA
        for li in poly.loop_indices:
            ca.data[li].color = col


def bake_basecolor(obj):
    img = bpy.data.images.new("AlbedoBake", width=BAKE_SIZE, height=BAKE_SIZE)
    img.colorspace_settings.name = "sRGB"

    mat = bpy.data.materials.new(name="Mat_AlbedoBake")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    attr = nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "Col"
    attr.attribute_type = "GEOMETRY"
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = img
    links.new(attr.outputs["Color"], bsdf.inputs["Base Color"])
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    obj.data.materials.clear()
    obj.data.materials.append(mat)
    nodes.active = tex

    bpy.context.scene.render.engine = "CYCLES"
    bpy.context.scene.cycles.device = "CPU"
    bpy.context.scene.cycles.samples = 16

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, margin=4)

    img.filepath_raw = BASECOLOR_PATH
    img.file_format = "PNG"
    img.save()
    print("OK ->", BASECOLOR_PATH)


def setup_normal_bake_material(obj, img_name):
    mat = bpy.data.materials.new(name="Mat_NormalBake")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.new(img_name, width=BAKE_SIZE, height=BAKE_SIZE, alpha=True)
    tex.image.colorspace_settings.name = "Non-Color"
    bsdf.inputs["Base Color"].default_value = (0.25, 0.2, 0.15, 1.0)
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active = obj
    bpy.context.object.active_material = mat
    nodes.active = tex


def make_high(obj_low):
    bpy.ops.object.select_all(action="DESELECT")
    obj_low.select_set(True)
    bpy.context.view_layer.objects.active = obj_low
    bpy.ops.object.duplicate()
    high = bpy.context.active_object
    high.name = "BlackjackTable_High"

    b_mod = high.modifiers.new(name="Bevel", type="BEVEL")
    b_mod.width = BEVEL_WIDTH_CM
    b_mod.segments = 3
    b_mod.limit_method = "ANGLE"
    b_mod.angle_limit = math.radians(30.0)

    s_mod = high.modifiers.new(name="Subdivision", type="SUBSURF")
    s_mod.levels = 2
    s_mod.render_levels = 2

    bpy.ops.object.modifier_apply(modifier="Bevel")
    bpy.ops.object.modifier_apply(modifier="Subdivision")
    return high


def bake_normal(low, high):
    bpy.ops.object.select_all(action="DESELECT")
    high.select_set(True)
    low.select_set(True)
    bpy.context.view_layer.objects.active = low

    bpy.context.scene.render.engine = "CYCLES"
    bpy.context.scene.cycles.device = "CPU"
    bpy.context.scene.cycles.samples = 32

    bpy.ops.object.bake(
        type="NORMAL",
        use_selected_to_active=True,
        cage_extrusion=CAGE_EXTRUSION,
        normal_space="TANGENT",
        normal_r="POS_X",
        normal_g="POS_Y",
        normal_b="POS_Z",
        margin=4,
    )

    mat = low.data.materials[0]
    for n in mat.node_tree.nodes:
        if n.type == "TEX_IMAGE" and n.image:
            n.image.filepath_raw = NORMAL_PATH
            n.image.file_format = "PNG"
            n.image.save()
            break


def export_fbx(obj):
    obj.data.materials.clear()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=FBX_PATH,
        use_selection=True,
        apply_scale_options="FBX_SCALE_ALL",
        global_scale=1.0,
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        use_mesh_modifiers=True,
        bake_space_transform=False,
    )


def main():
    clear_scene()
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.length_unit = "CENTIMETERS"

    low = create_table_mesh()
    unwrap_mesh(low)
    store_vertex_colors_from_material_slots(low)

    try:
        bake_basecolor(low)
    except Exception as e:
        print("Bake BaseColor échoué :", e)

    setup_normal_bake_material(low, "NormalBake")
    high = make_high(low)

    try:
        bake_normal(low, high)
    except Exception as e:
        print("Bake Normal échoué :", e)

    bpy.ops.object.select_all(action="DESELECT")
    high.select_set(True)
    bpy.ops.object.delete()

    export_fbx(low)

    print("OK ->", FBX_PATH)
    print("OK ->", NORMAL_PATH)


if __name__ == "__main__":
    main()
