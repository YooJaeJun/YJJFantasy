"""주인공 고양이 '냥용사'를 Blender로 만든다: 모델 + 뼈대 + 애니메이션 -> GLB.

그림체: 귀엽지만 엉뚱한 2D 모바일 게임 고양이의 3D화. 큰 머리, 짧은 팔다리, 무표정한 점 눈, ω 입.
외곽선은 게임의 셀 셰이딩 포스트 프로세스가 그린다.

실행:
  blender -b --factory-startup --python Tools/Character/make_hero.py -- [--preview]
결과:
  Tools/Character/Generated/SK_Hero.glb
  Tools/Character/Generated/preview_*.png (--preview일 때 애니메이션 자세 확인용)

좌표: Blender Z 위, 캐릭터는 -Y를 본다. 키 약 1.55m.
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
FPS = 30

# ------------------------------------------------------------------ 치수 (m)
HEAD_C = Vector((0, 0, 1.08))
HEAD_R = Vector((0.50, 0.43, 0.40))
BODY_C = Vector((0, 0, 0.50))
BODY_R = Vector((0.27, 0.24, 0.30))
LEG_X = 0.14
ARM_TOP = Vector((0.24, 0, 0.64))
ARM_END = Vector((0.31, -0.02, 0.42))

# ------------------------------------------------------------------ 색
WHITE = (0.96, 0.94, 0.90, 1)
INK = (0.03, 0.03, 0.04, 1)
PINK = (1.0, 0.55, 0.62, 1)
SCARF = (0.92, 0.25, 0.22, 1)
LEAF = (0.35, 0.75, 0.3, 1)


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = FPS


def material(name, color, rough=0.8):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = rough
    m.diffuse_color = color
    return m


def new_obj(name, bm, mat, group=None):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    me.materials.append(mat)
    for p in me.polygons:
        p.use_smooth = True
    if group:
        vg = ob.vertex_groups.new(name=group)
        vg.add(list(range(len(me.vertices))), 1.0, "REPLACE")
    return ob


def ellipsoid(center, radii, rot=None, segs=32, rings=16):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0)
    m = Matrix.Translation(center) @ (rot.to_matrix().to_4x4() if rot else Matrix()) @ Matrix.Diagonal((*radii, 1))
    bmesh.ops.transform(bm, matrix=m, verts=bm.verts)
    return bm


def capsule(a, b, r):
    """a-b 사이 캡슐 (구 두 개 + 원기둥). 리메시로 합쳐지니 대충 겹치면 된다"""
    bm = bmesh.new()
    d = b - a
    rot = d.to_track_quat("Z", "Y")
    for p in (a, b):
        t = ellipsoid(p, (r, r, r), segs=16, rings=8)
        _merge(bm, t)
    cyl = bmesh.new()
    bmesh.ops.create_cone(cyl, cap_ends=True, segments=16, radius1=r, radius2=r, depth=d.length)
    bmesh.ops.transform(cyl, matrix=Matrix.Translation((a + b) / 2) @ rot.to_matrix().to_4x4(), verts=cyl.verts)
    _merge(bm, cyl)
    return bm


def cone(base, tip, r):
    bm = bmesh.new()
    d = tip - base
    bmesh.ops.create_cone(bm, cap_ends=True, segments=20, radius1=r, radius2=0.0, depth=d.length)
    rot = d.to_track_quat("Z", "Y")
    bmesh.ops.transform(bm, matrix=Matrix.Translation((base + tip) / 2) @ rot.to_matrix().to_4x4(), verts=bm.verts)
    return bm


def _merge(dst, src):
    me = bpy.data.meshes.new("tmp")
    src.to_mesh(me)
    src.free()
    dst.from_mesh(me)
    bpy.data.meshes.remove(me)


def head_surface(x, z, inset=0.0):
    """머리 타원체 앞면(-Y)의 점과 법선"""
    nx, nz = x / HEAD_R.x, (z - HEAD_C.z) / HEAD_R.z
    ny = math.sqrt(max(0.0, 1 - nx * nx - nz * nz))
    p = Vector((x, -ny * HEAD_R.y, z))
    n = Vector((p.x / HEAD_R.x ** 2, (p.y) / HEAD_R.y ** 2, (p.z - HEAD_C.z) / HEAD_R.z ** 2)).normalized()
    return p - n * inset, n


def on_head(x, z, radii, inset):
    p, n = head_surface(x, z, inset)
    # 납작한 축(Y)을 법선에 맞춘다
    rot = Vector((0, -1, 0)).rotation_difference(n)
    return ellipsoid(p, radii, rot, segs=20, rings=10)


def arc(center, radius, thick, start, end, normal_rot):
    """ω 입의 반원 하나 (토러스 일부)"""
    bm = bmesh.new()
    steps = 10
    prev = None
    for i in range(steps + 1):
        a = start + (end - start) * i / steps
        p = Vector((math.cos(a) * radius, 0, math.sin(a) * radius))
        s = ellipsoid(Vector(), (thick, thick, thick), segs=8, rings=6)
        bmesh.ops.transform(s, matrix=Matrix.Translation(center + normal_rot @ p), verts=s.verts)
        _merge(bm, s)
    return bm


# ------------------------------------------------------------------ 몸
def build_body(mats):
    bm = ellipsoid(HEAD_C, HEAD_R, segs=48, rings=24)
    _merge(bm, ellipsoid(BODY_C, BODY_R))
    # 목: 머리와 몸을 자연스럽게
    _merge(bm, ellipsoid(Vector((0, 0, 0.74)), (0.2, 0.18, 0.12)))
    for s in (-1, 1):
        _merge(bm, capsule(Vector((s * LEG_X, 0, 0.28)), Vector((s * LEG_X, -0.01, 0.07)), 0.085))
        _merge(bm, ellipsoid(Vector((s * LEG_X, -0.04, 0.05)), (0.1, 0.13, 0.06)))  # 발
        _merge(bm, capsule(Vector((s * ARM_TOP.x, 0, ARM_TOP.z)), Vector((s * ARM_END.x, ARM_END.y, ARM_END.z)), 0.07))
    # 꼬리: 뒤로 뻗었다가 위로 휘는 곡선
    pts = [Vector((0, 0.2, 0.33)), Vector((0, 0.36, 0.40)), Vector((0, 0.46, 0.55)), Vector((0, 0.47, 0.72))]
    for a, b in zip(pts, pts[1:]):
        _merge(bm, capsule(a, b, 0.05))

    ob = new_obj("Body", bm, mats["white"])
    # 겹친 덩어리를 하나의 매끈한 표면으로
    rm = ob.modifiers.new("Remesh", "REMESH")
    rm.mode = "VOXEL"
    rm.voxel_size = 0.018
    sm = ob.modifiers.new("Smooth", "SMOOTH")
    sm.factor = 0.6
    sm.iterations = 8
    dec = ob.modifiers.new("Decimate", "DECIMATE")
    dec.ratio = 0.35
    apply_modifiers(ob)
    for p in ob.data.polygons:
        p.use_smooth = True
    return ob


def apply_modifiers(ob):
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
    ob.modifiers.clear()
    old = ob.data
    ob.data = me
    bpy.data.meshes.remove(old)
    me.materials.clear()
    me.materials.append(bpy.data.materials["White"])


def build_face_and_props(mats):
    parts = []
    # 눈: 세로로 긴 점 (무표정)
    for s in (-1, 1):
        parts.append(new_obj("Eye", on_head(s * 0.17, 1.13, (0.032, 0.02, 0.062), 0.006), mats["ink"], "head"))
        parts.append(new_obj("Blush", on_head(s * 0.29, 1.02, (0.075, 0.012, 0.04), 0.004), mats["pink"], "head"))
    # ω 입
    p, n = head_surface(0.0, 1.01, 0.004)
    rot = Vector((0, -1, 0)).rotation_difference(n).to_matrix()
    for s in (-1, 1):
        parts.append(new_obj("Mouth", arc(p + rot @ Vector((s * 0.033, 0, 0.0)), 0.033, 0.009, math.pi * 1.05, math.pi * 1.95, rot), mats["ink"], "head"))
    # 귀: 바깥쪽으로 살짝 벌어진 원뿔 + 분홍 속귀
    for s in (-1, 1):
        base = Vector((s * 0.27, 0.02, 1.36))
        tip = Vector((s * 0.40, 0.04, 1.62))
        parts.append(new_obj("Ear", cone(base, tip, 0.14), mats["white"], "ear_" + ("l" if s > 0 else "r")))
        ib = base + Vector((0, -0.05, 0.02))
        parts.append(new_obj("EarIn", cone(ib, ib + (tip - base) * 0.8, 0.08), mats["pink"], "ear_" + ("l" if s > 0 else "r")))
    # 빨간 스카프 + 등 뒤로 늘어진 꼬리
    tor = bmesh.new()
    bmesh.ops.create_circle(tor, segments=24, radius=0.24)
    torus = bmesh.new()
    for v in tor.verts:
        s = ellipsoid(Vector((v.co.x, v.co.y * 0.92, 0.77)), (0.065, 0.065, 0.055), segs=10, rings=6)
        _merge(torus, s)
    tor.free()
    parts.append(new_obj("Scarf", torus, mats["scarf"], "spine"))
    tail = ellipsoid(Vector((0.1, 0.25, 0.64)), (0.07, 0.03, 0.15), Quaternion((0, 1, 0), 0.35))
    parts.append(new_obj("ScarfTail", tail, mats["scarf"], "spine"))
    # 머리 위 새싹 (엉뚱한 포인트)
    stem = capsule(Vector((0.02, 0, 1.46)), Vector((0.04, 0, 1.56)), 0.012)
    parts.append(new_obj("Stem", stem, mats["leaf"], "head"))
    for s in (-1, 1):
        leaf = ellipsoid(Vector((0.04 + s * 0.05, 0, 1.58)), (0.055, 0.02, 0.025), Quaternion((0, 1, 0), -s * 0.4))
        parts.append(new_obj("Leaf", leaf, mats["leaf"], "head"))
    return parts


# ------------------------------------------------------------------ 뼈대
BONES = [
    # 이름, 머리, 꼬리, 부모
    ("root", (0, 0, 0), (0, 0, 0.12), None),
    ("pelvis", (0, 0, 0.32), (0, 0, 0.52), "root"),
    ("spine", (0, 0, 0.52), (0, 0, 0.74), "pelvis"),
    ("head", (0, 0, 0.74), (0, 0, 1.30), "spine"),
    ("ear_l", (0.27, 0.02, 1.36), (0.40, 0.04, 1.62), "head"),
    ("ear_r", (-0.27, 0.02, 1.36), (-0.40, 0.04, 1.62), "head"),
    ("arm_l", tuple(ARM_TOP), tuple(ARM_END), "spine"),
    ("arm_r", (-ARM_TOP.x, ARM_TOP.y, ARM_TOP.z), (-ARM_END.x, ARM_END.y, ARM_END.z), "spine"),
    # 손: 무게 없는 끝 뼈. 게임에서 검을 쥐는 위치
    ("hand_l", tuple(ARM_END), tuple(ARM_END + Vector((0.02, -0.01, -0.08))), "arm_l"),
    ("hand_r", (-ARM_END.x, ARM_END.y, ARM_END.z), (-ARM_END.x - 0.02, ARM_END.y - 0.01, ARM_END.z - 0.08), "arm_r"),
    ("leg_l", (LEG_X, 0, 0.30), (LEG_X, 0, 0.04), "pelvis"),
    ("leg_r", (-LEG_X, 0, 0.30), (-LEG_X, 0, 0.04), "pelvis"),
    ("tail1", (0, 0.18, 0.33), (0, 0.36, 0.40), "pelvis"),
    ("tail2", (0, 0.36, 0.40), (0, 0.46, 0.55), "tail1"),
    ("tail3", (0, 0.46, 0.55), (0, 0.47, 0.74), "tail2"),
]


def build_armature():
    arm = bpy.data.armatures.new("Rig")
    ob = bpy.data.objects.new("SK_Hero", arm)
    bpy.context.scene.collection.objects.link(ob)
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for name, h, t, parent in BONES:
        b = arm.edit_bones.new(name)
        b.head = h
        b.tail = t
        b.roll = 0.0
        if parent:
            b.parent = arm.edit_bones[parent]
            b.use_connect = False
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in ob.pose.bones:
        pb.rotation_mode = "XYZ"
    return ob


def seg_dist(p, a, b):
    ab = b - a
    t = max(0.0, min(1.0, (p - a).dot(ab) / max(ab.length_squared, 1e-9)))
    return (p - (a + ab * t)).length


def smooth01(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def skin_body(body):
    """몸 덩어리 가중치. 부위를 나눠서 거의 단단하게 붙인다 (거리 섞기는 몸통이 팔다리에 끌려 구겨진다).
    머리 = 통째로, 팔·다리·꼬리 = 해당 뼈, 몸통 = 골반/등뼈를 높이로 섞고, 경계만 부드럽게."""
    groups = {name: body.vertex_groups.new(name=name) for name, *_ in BONES if name not in ("root", "ear_l", "ear_r", "hand_l", "hand_r")}
    seg = {name: (Vector(h), Vector(t)) for name, h, t, _ in BONES}
    for v in body.data.vertices:
        p = v.co
        w = {}
        # 몸통 기본: 골반 -> 등뼈 -> 머리 (높이로)
        up = smooth01((p.z - 0.42) / 0.2)
        neck = smooth01((p.z - 0.70) / 0.1)
        w["pelvis"] = (1 - up)
        w["spine"] = up * (1 - neck)
        w["head"] = neck
        # 팔: 팔 뼈 가까이 + 몸 바깥쪽
        side = "arm_l" if p.x > 0 else "arm_r"
        a, b = seg[side]
        d = seg_dist(p, a, b)
        arm = smooth01((0.11 - d) / 0.04) * smooth01((abs(p.x) - 0.18) / 0.06)
        # 다리: 몸통 아래
        side = "leg_l" if p.x > 0 else "leg_r"
        leg = smooth01((0.31 - p.z) / 0.07) * smooth01((abs(p.x) - 0.02) / 0.05) if p.y < 0.15 else 0.0
        # 꼬리: 등 뒤
        tail_w, tail_name = 0.0, None
        if p.y > 0.15:
            ds = sorted((seg_dist(p, *seg[n]), n) for n in ("tail1", "tail2", "tail3"))
            tail_w = smooth01((0.09 - ds[0][0]) / 0.03) * smooth01((p.y - 0.17) / 0.05)
            tail_name = ds[0][1]
        rest = max(0.0, 1.0 - arm - leg - tail_w)
        for k in list(w):
            w[k] *= rest
        if arm > 0:
            w["arm_l" if p.x > 0 else "arm_r"] = arm
        if leg > 0:
            w["leg_l" if p.x > 0 else "leg_r"] = leg
        if tail_w > 0:
            w[tail_name] = tail_w
        tot = sum(w.values()) or 1.0
        for n, x in w.items():
            if x > 1e-4:
                groups[n].add([v.index], x / tot, "REPLACE")


# ------------------------------------------------------------------ 애니메이션
def deg(x, y=0.0, z=0.0):
    return (math.radians(x), math.radians(y), math.radians(z))


def make_action(rig, name, length, keys, loop=True):
    """keys: {bone: [(frame, (rx,ry,rz) 도, (lx,ly,lz) 또는 None), ...]}"""
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = act
    for pb in rig.pose.bones:
        pb.rotation_euler = (0, 0, 0)
        pb.location = (0, 0, 0)
    for bone, frames in keys.items():
        pb = rig.pose.bones[bone]
        for f, rot, loc in frames:
            pb.rotation_euler = deg(*rot)
            pb.keyframe_insert("rotation_euler", frame=f)
            if loc is not None:
                pb.location = loc
                pb.keyframe_insert("location", frame=f)
    act.frame_range = (0, length)
    act.use_frame_range = True
    return act


def cycle(bone_keys, length):
    """첫 키를 마지막 프레임에 복사해 매끈하게 반복"""
    out = {}
    for bone, frames in bone_keys.items():
        f0 = frames[0]
        out[bone] = frames + [(length, f0[1], f0[2])]
    return out


def build_actions(rig):
    """뼈 축 (모두 roll 0):
    - 세로로 선 뼈(골반·등·머리·귀): X+ = 앞으로 숙임, Y = 제자리 돌기(yaw), Z = 옆으로 기울기
    - 아래로 뻗은 뼈(다리·팔): X- = 앞으로 뻗기, Z = 옆으로 들기 (왼팔은 -, 오른팔은 +가 바깥)
    - 뒤로 뻗은 꼬리: X+ = 위로, Z = 좌우 흔들기
    """
    acts = []
    # 대기: 숨쉬기, 꼬리 살랑, 가끔 귀 쫑긋
    L = 60
    acts.append(make_action(rig, "Idle", L, cycle({
        "pelvis": [(0, (0, 0, 0), (0, 0, 0)), (30, (0, 0, 0), (0, 0.012, 0))],
        "head": [(0, (0, 0, 2), None), (30, (2, 0, -2), None)],
        "tail1": [(0, (0, 0, 15), None), (30, (0, 0, -15), None)],
        "tail2": [(0, (0, 0, 20), None), (30, (0, 0, -20), None)],
        "ear_l": [(0, (0, 0, 0), None), (40, (0, 0, 0), None), (44, (-25, 0, 0), None), (48, (0, 0, 0), None)],
        "arm_l": [(0, (0, 0, -4), None), (30, (0, 0, -8), None)],
        "arm_r": [(0, (0, 0, 4), None), (30, (0, 0, 8), None)],
    }, L)))
    # 걷기: 뒤뚱뒤뚱 (몸을 좌우로 기울이며)
    L = 24
    acts.append(make_action(rig, "Walk", L, cycle({
        "leg_l": [(0, (-35, 0, 0), None), (12, (35, 0, 0), None)],
        "leg_r": [(0, (35, 0, 0), None), (12, (-35, 0, 0), None)],
        "arm_l": [(0, (30, 0, -10), None), (12, (-30, 0, -10), None)],
        "arm_r": [(0, (-30, 0, 10), None), (12, (30, 0, 10), None)],
        "pelvis": [(0, (0, 0, 7), (0, 0, 0)), (6, (0, 0, 0), (0, 0.04, 0)), (12, (0, 0, -7), (0, 0, 0)), (18, (0, 0, 0), (0, 0.04, 0))],
        "head": [(0, (0, 0, -4), None), (12, (0, 0, 4), None)],
        "tail1": [(0, (0, 0, 20), None), (12, (0, 0, -20), None)],
    }, L)))
    # 달리기: 앞으로 숙이고 팔을 미친 듯이 휘젓는다, 귀는 뒤로 눕는다
    L = 14
    acts.append(make_action(rig, "Run", L, cycle({
        "leg_l": [(0, (-65, 0, 0), None), (7, (55, 0, 0), None)],
        "leg_r": [(0, (55, 0, 0), None), (7, (-65, 0, 0), None)],
        "arm_l": [(0, (80, 0, -25), None), (7, (-80, 0, -25), None)],
        "arm_r": [(0, (-80, 0, 25), None), (7, (80, 0, 25), None)],
        "pelvis": [(0, (14, 0, 0), (0, 0, 0)), (3, (17, 0, 0), (0, 0.07, 0)), (7, (14, 0, 0), (0, 0, 0)), (10, (17, 0, 0), (0, 0.07, 0))],
        "head": [(0, (-6, 0, 0), None), (3, (-2, 0, 0), None), (7, (-6, 0, 0), None), (10, (-2, 0, 0), None)],
        "ear_l": [(0, (-40, 0, 0), None)],
        "ear_r": [(0, (-40, 0, 0), None)],
        "tail1": [(0, (30, 0, 0), None)],
        "tail2": [(0, (-20, 0, 0), None)],
    }, L)))
    # 점프: 만세!
    L = 12
    acts.append(make_action(rig, "Jump", L, {
        "arm_l": [(0, (0, 0, -20), None), (6, (0, 0, -125), None)],
        "arm_r": [(0, (0, 0, 20), None), (6, (0, 0, 125), None)],
        "leg_l": [(0, (0, 0, 0), None), (6, (-50, 0, 0), None)],
        "leg_r": [(0, (0, 0, 0), None), (6, (30, 0, 0), None)],
        "head": [(0, (0, 0, 0), None), (6, (-12, 0, 0), None)],
        "ear_l": [(0, (0, 0, 0), None), (6, (-30, 0, 0), None)],
        "ear_r": [(0, (0, 0, 0), None), (6, (-30, 0, 0), None)],
    }, loop=False))
    # 낙하: 팔다리 버둥버둥
    L = 16
    acts.append(make_action(rig, "Fall", L, cycle({
        "arm_l": [(0, (0, 0, -95), None), (8, (0, 0, -135), None)],
        "arm_r": [(0, (0, 0, 135), None), (8, (0, 0, 95), None)],
        "leg_l": [(0, (-30, 0, 0), None), (8, (30, 0, 0), None)],
        "leg_r": [(0, (30, 0, 0), None), (8, (-30, 0, 0), None)],
        "ear_l": [(0, (-40, 0, 0), None), (8, (-15, 0, 0), None)],
        "ear_r": [(0, (-15, 0, 0), None), (8, (-40, 0, 0), None)],
        "tail1": [(0, (-30, 0, 0), None), (8, (-10, 0, 0), None)],
    }, L)))
    # 활공: 두 팔로 날개를 붙잡고 다리는 대롱대롱, 꼬리는 뒤로 나부낀다
    L = 40
    acts.append(make_action(rig, "Glide", L, cycle({
        "arm_l": [(0, (0, 0, -115), None), (20, (0, 0, -105), None)],
        "arm_r": [(0, (0, 0, 115), None), (20, (0, 0, 105), None)],
        "leg_l": [(0, (20, 0, 0), None), (20, (35, 0, 0), None)],
        "leg_r": [(0, (35, 0, 0), None), (20, (20, 0, 0), None)],
        "pelvis": [(0, (12, 0, 0), (0, 0, 0))],
        "head": [(0, (-10, 0, 0), None)],
        "ear_l": [(0, (-60, 0, 0), None), (20, (-50, 0, 0), None)],
        "ear_r": [(0, (-50, 0, 0), None), (20, (-60, 0, 0), None)],
        "tail1": [(0, (-15, 0, 10), None), (20, (-5, 0, -10), None)],
        "tail2": [(0, (-30, 0, 0), None)],
    }, L)))
    # 공격 세 가지: 오른쪽에서 왼쪽으로, 반대로, 빙글 돌며 내려찍기
    L = 10
    acts.append(make_action(rig, "Attack1", L, {
        "arm_r": [(0, (-20, 0, 110), None), (3, (-90, 0, 80), None), (6, (-80, 0, -40), None), (10, (0, 0, 10), None)],
        "spine": [(0, (0, -25, 0), None), (4, (0, 30, 0), None), (10, (0, 0, 0), None)],
        "head": [(0, (0, 10, 0), None), (4, (0, -10, 0), None), (10, (0, 0, 0), None)],
    }, loop=False))
    acts.append(make_action(rig, "Attack2", L, {
        "arm_r": [(0, (-80, 0, -40), None), (3, (-90, 0, 0), None), (6, (-30, 0, 120), None), (10, (0, 0, 10), None)],
        "spine": [(0, (0, 30, 0), None), (4, (0, -30, 0), None), (10, (0, 0, 0), None)],
    }, loop=False))
    acts.append(make_action(rig, "Attack3", 12, {
        "arm_r": [(0, (-170, 0, 0), None), (5, (-170, 0, 0), None), (8, (-20, 0, 0), None), (12, (0, 0, 10), None)],
        "pelvis": [(0, (0, 0, 0), (0, 0, 0)), (4, (0, 180, 0), (0, 0.15, 0)), (8, (0, 360, 0), (0, 0, 0)), (12, (0, 360, 0), (0, 0, 0))],
        "leg_l": [(0, (0, 0, 0), None), (4, (-40, 0, 0), None), (8, (0, 0, 0), None)],
        "leg_r": [(0, (0, 0, 0), None), (4, (-40, 0, 0), None), (8, (0, 0, 0), None)],
    }, loop=False))
    # 아야: 머리 젖히고 팔다리 쫙
    acts.append(make_action(rig, "Hurt", 12, {
        "head": [(0, (0, 0, 0), None), (3, (-25, 0, 0), None), (12, (0, 0, 0), None)],
        "spine": [(0, (0, 0, 0), None), (3, (-15, 0, 0), None), (12, (0, 0, 0), None)],
        "arm_l": [(0, (0, 0, 0), None), (3, (0, 0, -100), None), (12, (0, 0, 0), None)],
        "arm_r": [(0, (0, 0, 0), None), (3, (0, 0, 100), None), (12, (0, 0, 0), None)],
        "ear_l": [(0, (0, 0, 0), None), (3, (-50, 0, 0), None), (12, (0, 0, 0), None)],
        "ear_r": [(0, (0, 0, 0), None), (3, (-50, 0, 0), None), (12, (0, 0, 0), None)],
    }, loop=False))
    rig.animation_data.action = None
    for pb in rig.pose.bones:
        pb.rotation_euler = (0, 0, 0)
        pb.location = (0, 0, 0)
    return acts


# ------------------------------------------------------------------ 미리보기
def render_previews(rig, acts):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_object_outline = True
    scene.render.resolution_x = 360
    scene.render.resolution_y = 420
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.75, 0.95)
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 2.3
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    views = {"front": (Vector((0, -5, 0.85)), 0), "side": (Vector((5, 0, 0.85)), 90), "q": (Vector((3.5, -3.5, 1.6)), 45)}
    for act in [None] + acts:
        rig.animation_data.action = act
        frames = [0] if act is None else sorted({0, int(act.frame_range[1] * 0.25), int(act.frame_range[1] * 0.5), int(act.frame_range[1] * 0.75)})
        for f in frames:
            scene.frame_set(f)
            for vname, (pos, _yaw) in views.items():
                if act is not None and vname != "q":
                    continue
                cam.location = pos
                d = Vector((0, 0, 0.8)) - pos
                cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
                scene.render.filepath = os.path.join(OUT, f"preview_{act.name if act else 'Rest'}_{vname}_{f:02d}.png")
                bpy.ops.render.render(write_still=True)
    rig.animation_data.action = None


def main():
    reset_scene()
    mats = {
        "white": material("White", WHITE),
        "ink": material("Ink", INK, 0.3),
        "pink": material("Pink", PINK),
        "scarf": material("Scarf", SCARF),
        "leaf": material("Leaf", LEAF),
    }
    body = build_body(mats)
    rig = build_armature()
    skin_body(body)
    parts = build_face_and_props(mats)

    # 한 메시로 합치고 뼈대에 붙인다
    bpy.ops.object.select_all(action="DESELECT")
    for ob in [body] + parts:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    body.name = "SK_Hero_Mesh"
    body.parent = rig
    mod = body.modifiers.new("Armature", "ARMATURE")
    mod.object = rig
    print(f"[Hero] verts {len(body.data.vertices)} tris ~{sum(len(p.vertices) - 2 for p in body.data.polygons)}")

    acts = build_actions(rig)
    print("[Hero] actions", [a.name for a in acts])
    if PREVIEW:
        render_previews(rig, acts)

    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = rig
    path = os.path.join(OUT, "SK_Hero.glb")
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLB",
        use_selection=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
        export_force_sampling=True,
        export_skins=True,
        export_yup=True,
    )
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SK_Hero.blend"))
    print("[Hero] ->", path)


main()
