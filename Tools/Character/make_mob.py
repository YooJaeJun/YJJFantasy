"""몬스터 '떡무지'를 Blender로 만든다: 납작한 찹쌀떡 몸 + 짧은 발 + 졸린 눈 + 화난 굵은 눈썹.

몸(SM_Mob_Body)과 얼굴(SM_Mob_Face)을 따로 내보낸다. 게임이 몸에 섬마다 다른 색을 입히고,
통통 튀는 움직임은 코드가 크기·높이로 준다(뼈대 없음).

실행: blender -b --factory-startup --python Tools/Character/make_mob.py -- [--preview]
좌표: Z 위, -Y가 앞. 바닥 가운데가 원점. 폭 약 1.3m.
"""

import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "Generated")
os.makedirs(OUT, exist_ok=True)
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
PREVIEW = "--preview" in ARGS

BODY_C = Vector((0, 0, 0.5))
BODY_R = Vector((0.65, 0.58, 0.46))


def material(name, color, rough=0.8):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = rough
    m.diffuse_color = color
    return m


def _merge(dst, src):
    me = bpy.data.meshes.new("tmp")
    src.to_mesh(me)
    src.free()
    dst.from_mesh(me)
    bpy.data.meshes.remove(me)


def ellipsoid(center, radii, rot=None, segs=32, rings=16):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0)
    m = Matrix.Translation(center) @ (rot.to_matrix().to_4x4() if rot else Matrix()) @ Matrix.Diagonal((*radii, 1))
    bmesh.ops.transform(bm, matrix=m, verts=bm.verts)
    return bm


def new_obj(name, bm, mat):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    me.materials.append(mat)
    for p in me.polygons:
        p.use_smooth = True
    return ob


def apply_modifiers(ob, mat):
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
    ob.modifiers.clear()
    old = ob.data
    ob.data = me
    bpy.data.meshes.remove(old)
    me.materials.clear()
    me.materials.append(mat)
    for p in me.polygons:
        p.use_smooth = True


def surface(x, z, inset=0.0):
    """몸 타원체 앞면(-Y)의 점과 법선"""
    nx, nz = x / BODY_R.x, (z - BODY_C.z) / BODY_R.z
    ny = math.sqrt(max(0.0, 1 - nx * nx - nz * nz))
    p = Vector((x, -ny * BODY_R.y, z))
    n = Vector((p.x / BODY_R.x ** 2, p.y / BODY_R.y ** 2, (p.z - BODY_C.z) / BODY_R.z ** 2)).normalized()
    return p - n * inset, n


def on_body(x, z, radii, inset, spin=0.0):
    p, n = surface(x, z, inset)
    rot = Vector((0, -1, 0)).rotation_difference(n) @ Quaternion((0, -1, 0), spin)
    return ellipsoid(p, radii, rot, segs=20, rings=10)


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    white = material("MobBody", (0.95, 0.95, 0.95, 1))
    ink = material("MobInk", (0.03, 0.03, 0.04, 1), 0.3)
    pink = material("MobPink", (1.0, 0.55, 0.62, 1))

    # 몸: 아래가 펑퍼짐한 떡 + 짧은 발 넷 + 정수리 뾰족 털
    bm = ellipsoid(BODY_C, BODY_R, segs=48, rings=24)
    _merge(bm, ellipsoid(Vector((0, 0, 0.22)), (0.72, 0.64, 0.2)))
    for sx in (-1, 1):
        for sy in (-1, 1):
            _merge(bm, ellipsoid(Vector((sx * 0.36, sy * 0.28 - 0.04, 0.07)), (0.13, 0.15, 0.08)))
    _merge(bm, ellipsoid(Vector((0.02, 0.05, 0.97)), (0.06, 0.06, 0.12), Quaternion((0, 1, 0), 0.4)))
    body = new_obj("SM_Mob_Body", bm, white)
    rm = body.modifiers.new("Remesh", "REMESH")
    rm.mode = "VOXEL"
    rm.voxel_size = 0.02
    sm = body.modifiers.new("Smooth", "SMOOTH")
    sm.factor = 0.6
    sm.iterations = 8
    dec = body.modifiers.new("Decimate", "DECIMATE")
    dec.ratio = 0.3
    apply_modifiers(body, white)

    # 얼굴: 반쯤 감긴 눈(위가 잘린 반원) + 화난 굵은 눈썹 + 작은 입 + 볼
    face = []
    for s in (-1, 1):
        face.append(new_obj("Eye", on_body(s * 0.2, 0.55, (0.075, 0.02, 0.045), 0.006), ink))
        # 눈꺼풀: 눈 위를 몸 색이 덮어 졸린 눈
        face.append(new_obj("Lid", on_body(s * 0.2, 0.585, (0.095, 0.03, 0.04), 0.0), white))
        face.append(new_obj("Brow", on_body(s * 0.2, 0.68, (0.12, 0.025, 0.028), 0.0, spin=s * 0.35), ink))
        face.append(new_obj("Cheek", on_body(s * 0.38, 0.45, (0.08, 0.012, 0.045), 0.004), pink))
    face.append(new_obj("Mouth", on_body(0.0, 0.43, (0.05, 0.015, 0.012), 0.004), ink))
    bpy.ops.object.select_all(action="DESELECT")
    for ob in face:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = face[0]
    bpy.ops.object.join()
    face_ob = bpy.context.view_layer.objects.active
    face_ob.name = "SM_Mob_Face"
    return body, face_ob


def export(ob, name):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    path = os.path.join(OUT, name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_animations=False, export_yup=True)
    print("[Mob] ->", path, len(ob.data.vertices))


def preview():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_object_outline = True
    scene.render.resolution_x = 420
    scene.render.resolution_y = 360
    scene.world = bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.75, 0.95)
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 1.9
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    for vname, pos in {"front": Vector((0, -5, 0.5)), "q": Vector((3, -3.5, 1.8))}.items():
        cam.location = pos
        cam.rotation_euler = (Vector((0, 0, 0.45)) - pos).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = os.path.join(OUT, f"preview_Mob_{vname}.png")
        bpy.ops.render.render(write_still=True)


body, face = build()
if PREVIEW:
    preview()
export(body, "SM_Mob_Body")
export(face, "SM_Mob_Face")
