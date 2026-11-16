"""에디터 Python(커맨드릿)으로 게임에 필요한 에셋을 만든다. 여러 번 실행해도 같은 결과.

만드는 것:
  /Game/YJJFantasy/Materials/M_YJJ_Surface   섬·바위·나무용 스타일라이즈드 표면 (윗면/옆면 색 자동 분리, 얼룩 변화)
  /Game/YJJFantasy/Materials/M_YJJ_Glow      빛기둥·바람기둥·별가루용 가산 발광
  /Game/YJJFantasy/Materials/M_YJJ_Water     폭포·호수용 반투명 물
  /Game/YJJFantasy/Materials/PP_YJJ_Toon     애니메 셀 셰이딩 + 외곽선 포스트 프로세스
  /Game/YJJFantasy/AI/<이름>                 Tools/AI3D/Generated/*/<이름>.glb 임포트 (있으면)
  /Game/YJJFantasy/Audio/*                   Tools/Audio/Generated/*.wav (generate_audio.py가 합성)
  /Game/YJJFantasy/Maps/L_Prototype          빈 맵 (게임모드가 하늘섬 월드를 생성)

실행: powershell -File Tools/Content/BuildContent.ps1 (에디터를 끈 상태)
"""

import glob
import os

import unreal

ROOT = "/Game/YJJFantasy"
MAT_DIR = ROOT + "/Materials"
AI_DIR = ROOT + "/AI"
MAP_DIR = ROOT + "/Maps"

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
AI_SRC = os.path.join(PROJECT_DIR, "Tools", "AI3D", "Generated")

mel = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(msg):
    unreal.log("[BuildContent] " + msg)


def new_material(name):
    path = f"{MAT_DIR}/{name}"
    if assets.does_asset_exist(path):
        assets.delete_asset(path)
    material = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError("create failed " + path)
    return material


def node(material, cls, x, y):
    return mel.create_material_expression(material, cls, x, y)


def vector_param(material, name, color, x, y):
    p = node(material, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*color))
    return p


def scalar_param(material, name, value, x, y):
    p = node(material, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def custom(material, code, inputs, output_type, x, y, desc):
    c = node(material, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code)
    c.set_editor_property("description", desc)
    c.set_editor_property("output_type", output_type)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    c.set_editor_property("inputs", pins)
    return c


def connect(src, src_pin, dst, dst_pin):
    if not mel.connect_material_expressions(src, src_pin, dst, dst_pin):
        raise RuntimeError(f"connect failed -> {dst_pin}")


def output(src, src_pin, prop):
    if not mel.connect_material_property(src, src_pin, prop):
        raise RuntimeError(f"output failed {prop}")


def finish(material):
    # 풀·꽃·빛 먼지는 인스턴스 메시로 그리므로 사용 플래그가 없으면 게임에서 기본 머티리얼로 바뀐다
    if material.get_editor_property("material_domain") == unreal.MaterialDomain.MD_SURFACE:
        material.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(material)
    assets.save_loaded_asset(material)
    log("material " + material.get_path_name())


# ---------------------------------------------------------------- 표면

SURFACE_CODE = r"""
// 윗면은 TopColor(풀), 옆·아랫면은 SideColor(바위). 경계는 약간 흔들어서 손그림 느낌
float3 n = normalize(N);
float wobble = sin(WP.x * 0.004 + WP.y * 0.003) * 0.08 + sin(WP.y * 0.011 - WP.z * 0.006) * 0.05;
float top = saturate((n.z - 0.55 + wobble) * Sharp);
float3 col = lerp(Side, Top, top);
// 큼직한 얼룩으로 색을 조금씩 바꿔서 단색 플라스틱처럼 보이지 않게
float3 cell = floor(WP / 180.0);
float h = frac(sin(dot(cell, float3(12.9898, 78.233, 37.719))) * 43758.5453);
float soft = sin(WP.x * 0.0021) * sin(WP.y * 0.0017) * 0.5 + 0.5;
col *= lerp(0.92, 1.08, h * 0.5 + soft * 0.5);
// 아랫면은 살짝 어둡고 푸르게 (하늘 반사광 느낌)
col = lerp(col, col * float3(0.7, 0.78, 1.0), saturate(-n.z) * 0.6);
return col;
"""


def build_surface():
    m = new_material("M_YJJ_Surface")
    top = vector_param(m, "TopColor", (0.36, 0.72, 0.28, 1), -900, -200)
    side = vector_param(m, "SideColor", (0.55, 0.52, 0.5, 1), -900, 0)
    sharp = scalar_param(m, "TopSharpness", 6.0, -900, 200)
    emissive = scalar_param(m, "Emissive", 0.0, -900, 400)
    rough = scalar_param(m, "Roughness", 0.85, -900, 500)
    normal_ws = node(m, unreal.MaterialExpressionVertexNormalWS, -900, 300)
    world_pos = node(m, unreal.MaterialExpressionWorldPosition, -900, 350)

    c = custom(m, SURFACE_CODE, ["Top", "Side", "Sharp", "N", "WP"], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, "Stylized surface")
    connect(top, "", c, "Top")
    connect(side, "", c, "Side")
    connect(sharp, "", c, "Sharp")
    connect(normal_ws, "", c, "N")
    connect(world_pos, "", c, "WP")
    output(c, "", unreal.MaterialProperty.MP_BASE_COLOR)

    mul = node(m, unreal.MaterialExpressionMultiply, -250, 300)
    connect(c, "", mul, "A")
    connect(emissive, "", mul, "B")
    output(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    output(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    finish(m)


# ---------------------------------------------------------------- 발광

GLOW_CODE = r"""
// 가장자리가 밝은 빛 덩어리. Core는 가운데 밝기, Rim은 가장자리 밝기
float ndv = abs(dot(normalize(N), normalize(V)));
float rim = pow(1.0 - ndv, 2.0);
float flicker = 0.92 + 0.08 * sin(T * 3.1 + WP.x * 0.01 + WP.z * 0.013);
return Color * (Core * ndv + Rim * rim) * Intensity * Fade * flicker;
"""


def build_glow():
    m = new_material("M_YJJ_Glow")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    color = vector_param(m, "Color", (1.0, 0.85, 0.5, 1), -900, -200)
    intensity = scalar_param(m, "Intensity", 4.0, -900, 0)
    fade = scalar_param(m, "Fade", 1.0, -900, 100)
    core = scalar_param(m, "Core", 0.35, -900, 200)
    rim = scalar_param(m, "Rim", 1.0, -900, 300)
    normal_ws = node(m, unreal.MaterialExpressionVertexNormalWS, -900, 400)
    camera = node(m, unreal.MaterialExpressionCameraVectorWS, -900, 450)
    time = node(m, unreal.MaterialExpressionTime, -900, 500)
    world_pos = node(m, unreal.MaterialExpressionWorldPosition, -900, 550)

    c = custom(m, GLOW_CODE, ["Color", "Intensity", "Fade", "Core", "Rim", "N", "V", "T", "WP"],
               unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, "Glow")
    connect(color, "", c, "Color")
    connect(intensity, "", c, "Intensity")
    connect(fade, "", c, "Fade")
    connect(core, "", c, "Core")
    connect(rim, "", c, "Rim")
    connect(normal_ws, "", c, "N")
    connect(camera, "", c, "V")
    connect(time, "", c, "T")
    connect(world_pos, "", c, "WP")
    output(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(m)


# ---------------------------------------------------------------- 물

WATER_CODE = r"""
// 흐르는 물결 줄무늬 + 가장자리 흰 거품
float flow = sin(WP.z * 0.05 + T * Speed * 6.0 + sin(WP.x * 0.02) * 2.0) * 0.5 + 0.5;
float ripple = sin(WP.x * 0.03 + T * 1.7) * sin(WP.y * 0.027 - T * 1.3) * 0.5 + 0.5;
float ndv = abs(dot(normalize(N), normalize(V)));
float foam = pow(1.0 - ndv, 3.0);
float3 col = lerp(Deep, Shallow, saturate(flow * 0.6 + ripple * 0.4));
return lerp(col, float3(1, 1, 1), saturate(foam * 0.8 + pow(flow, 8.0) * 0.5));
"""


def build_water():
    m = new_material("M_YJJ_Water")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    deep = vector_param(m, "Deep", (0.05, 0.35, 0.65, 1), -900, -200)
    shallow = vector_param(m, "Shallow", (0.35, 0.85, 0.95, 1), -900, 0)
    speed = scalar_param(m, "Speed", 1.0, -900, 150)
    opacity = scalar_param(m, "Opacity", 0.8, -900, 250)
    bright = scalar_param(m, "Brightness", 1.6, -900, 350)
    normal_ws = node(m, unreal.MaterialExpressionVertexNormalWS, -900, 450)
    camera = node(m, unreal.MaterialExpressionCameraVectorWS, -900, 500)
    time = node(m, unreal.MaterialExpressionTime, -900, 550)
    world_pos = node(m, unreal.MaterialExpressionWorldPosition, -900, 600)

    c = custom(m, WATER_CODE, ["Deep", "Shallow", "Speed", "N", "V", "T", "WP"],
               unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, "Water")
    for pin, src in (("Deep", deep), ("Shallow", shallow), ("Speed", speed), ("N", normal_ws), ("V", camera), ("T", time), ("WP", world_pos)):
        connect(src, "", c, pin)
    mul = node(m, unreal.MaterialExpressionMultiply, -250, 0)
    connect(c, "", mul, "A")
    connect(bright, "", mul, "B")
    output(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    output(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


# ---------------------------------------------------------------- 셀 셰이딩 포스트 프로세스

TOON_CODE = r"""
// 조명을 두세 단계로 끊어서 애니메 셀 셰이딩처럼 보이게 하고, 깊이·노멀 경계에 색 외곽선을 그린다.
// SceneColor / BaseColor = 그 픽셀이 받은 빛. 이 빛만 단계화해서 다시 BaseColor를 곱한다.
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float3 col = C.rgb;
float depth = D.r;
if (depth > 2000000.0)
{
    return col; // 하늘
}

float3 albedo = max(B.rgb, 0.03);
float3 light = col / albedo;
float lum = dot(light, float3(0.3, 0.59, 0.11));
float rel = lum / max(Ref, 0.0001);

// 밝은 면 / 그늘 두 단계 + 아주 어두운 곳 한 단계. 경계는 살짝 부드럽게
float lit = smoothstep(0.42, 0.52, rel);
float deep = smoothstep(0.08, 0.16, rel);
float3 shadowTint = float3(0.62, 0.66, 0.9);
float3 band = lerp(shadowTint * lerp(0.55, 1.0, deep), float3(1, 1, 1), lit);
float3 toon = albedo * Ref * band;
// 해 반사광, 발광체처럼 기준보다 훨씬 밝은 빛은 그대로 살린다
toon += albedo * max(lum - Ref * 1.25, 0.0) * 0.6;
float3 result = lerp(col, toon, Strength);

// 외곽선: 주변 4픽셀과 깊이·노멀 차이
float2 px = Inv * Thickness;
float dL = SceneTextureLookup(uv + float2(-px.x, 0), 1, false).r;
float dR = SceneTextureLookup(uv + float2(px.x, 0), 1, false).r;
float dU = SceneTextureLookup(uv + float2(0, -px.y), 1, false).r;
float dDn = SceneTextureLookup(uv + float2(0, px.y), 1, false).r;
float dEdge = (abs(dL + dR - 2.0 * depth) + abs(dU + dDn - 2.0 * depth)) / depth;

float3 n0 = N.rgb;
float3 nL = SceneTextureLookup(uv + float2(-px.x, 0), 8, false).rgb;
float3 nR = SceneTextureLookup(uv + float2(px.x, 0), 8, false).rgb;
float3 nU = SceneTextureLookup(uv + float2(0, -px.y), 8, false).rgb;
float3 nDn = SceneTextureLookup(uv + float2(0, px.y), 8, false).rgb;
float nEdge = (1.0 - dot(n0, nL)) + (1.0 - dot(n0, nR)) + (1.0 - dot(n0, nU)) + (1.0 - dot(n0, nDn));

float edge = saturate(smoothstep(0.08, 0.2, dEdge) + smoothstep(0.6, 1.2, nEdge));
// 멀리 있는 건 외곽선을 흐리게 (지저분해지지 않게)
edge *= 1.0 - smoothstep(OutlineFar * 0.5, OutlineFar, depth);
float3 ink = result * float3(0.28, 0.24, 0.32);
return lerp(result, ink, edge * OutlineStrength);
"""


def scene_texture(material, texture_id, x, y):
    s = node(material, unreal.MaterialExpressionSceneTexture, x, y)
    s.set_editor_property("scene_texture_id", texture_id)
    return s


def build_toon_pp():
    m = new_material("PP_YJJ_Toon")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_DOF)

    color = scene_texture(m, unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -1000, -300)
    depth = scene_texture(m, unreal.SceneTextureId.PPI_SCENE_DEPTH, -1000, -100)
    normal = scene_texture(m, unreal.SceneTextureId.PPI_WORLD_NORMAL, -1000, 100)
    base = scene_texture(m, unreal.SceneTextureId.PPI_BASE_COLOR, -1000, 300)
    ref = scalar_param(m, "Ref", 1.0, -1000, 500)
    strength = scalar_param(m, "Strength", 0.65, -1000, 600)
    thickness = scalar_param(m, "Thickness", 1.2, -1000, 700)
    outline = scalar_param(m, "OutlineStrength", 0.85, -1000, 800)
    far = scalar_param(m, "OutlineFar", 9000.0, -1000, 900)

    c = custom(m, TOON_CODE, ["C", "D", "N", "B", "Inv", "Ref", "Strength", "Thickness", "OutlineStrength", "OutlineFar"],
               unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, "Toon")
    connect(color, "Color", c, "C")
    connect(depth, "Color", c, "D")
    connect(normal, "Color", c, "N")
    connect(base, "Color", c, "B")
    connect(depth, "InvSize", c, "Inv")
    connect(ref, "", c, "Ref")
    connect(strength, "", c, "Strength")
    connect(thickness, "", c, "Thickness")
    connect(outline, "", c, "OutlineStrength")
    connect(far, "", c, "OutlineFar")
    output(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(m)


# ---------------------------------------------------------------- AI 모델

def import_ai_models():
    files = sorted(glob.glob(os.path.join(AI_SRC, "*", "*.glb")))
    if not files:
        log("AI 모델 없음 (Tools/AI3D/Generated)")
        return
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        dest = f"{AI_DIR}/{name}"
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", dest)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        asset_tools.import_asset_tasks([task])
        # Nanite는 이 메시(10만 면, 4개 배치)에선 오히려 느렸다(102 -> 92 FPS). 기본 메시로 둔다
        log(f"AI 모델 {name}: {list(task.get_editor_property('imported_object_paths'))}")


AUDIO_DIR = ROOT + "/Audio"
AUDIO_SRC = os.path.join(PROJECT_DIR, "Tools", "Audio", "Generated")


def import_sounds():
    files = sorted(glob.glob(os.path.join(AUDIO_SRC, "*.wav")))
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", AUDIO_DIR)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        asset_tools.import_asset_tasks([task])
        sound = assets.load_asset(f"{AUDIO_DIR}/{name}")
        if sound is None:
            raise RuntimeError("sound import failed " + name)
        if name.startswith("BGM_") or name.startswith("AMB_"):
            sound.set_editor_property("looping", True)
        assets.save_loaded_asset(sound)
    log(f"sounds {len(files)}")


def make_map():
    path = MAP_DIR + "/L_Prototype"
    if assets.does_asset_exist(path):
        log("map exists " + path)
        return
    world = asset_tools.create_asset("L_Prototype", MAP_DIR, unreal.World, unreal.WorldFactory())
    assets.save_loaded_asset(world)
    log("map " + path)


def main():
    build_surface()
    build_glow()
    build_water()
    build_toon_pp()
    import_ai_models()
    import_sounds()
    make_map()
    assets.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
    log("done")


main()
