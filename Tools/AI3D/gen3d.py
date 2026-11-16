"""텍스트 또는 이미지로 3D 모델(GLB)을 만든다. 모두 무료 오픈 모델을 Hugging Face 무료 GPU(ZeroGPU)에서 돌린다.

  1) 텍스트 -> 컨셉 이미지 : FLUX.1-schnell (Apache-2.0)      black-forest-labs/FLUX.1-schnell
  2) 이미지 -> 3D(GLB)      : TRELLIS.2 (MIT, Microsoft)         microsoft/TRELLIS.2

사용 예:
  python gen3d.py --name SM_FloatingShrine --prompt "a small ancient floating island shrine with a waterfall"
  python gen3d.py --name SM_MyRock --image my_rock.png

결과: Tools/AI3D/Generated/<name>/concept.png, <name>.glb
HF_TOKEN 환경변수(무료 Hugging Face 계정 토큰)가 있으면 하루 GPU 할당량이 늘어난다. 없어도 익명으로 동작한다.
실행은 Gen3D.ps1 참고.
"""

import argparse
import os
import shutil
import sys
import time

from gradio_client import Client, handle_file

IMAGE_SPACE = "black-forest-labs/FLUX.1-schnell"
MESH_SPACE = "microsoft/TRELLIS.2"

# 모든 컨셉 이미지에 붙는 공통 스타일. 3D 변환이 잘 되도록 '단일 오브젝트, 단색 배경, 전신'을 강제한다
STYLE = (
    "stylized fantasy game asset, hand-painted textures, soft vibrant colors, "
    "single object centered, full view, isometric three-quarter angle, "
    "plain light gray background, no ground shadow, no text"
)

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_ROOT = os.path.join(HERE, "Generated")


def log(msg):
    print(f"[Gen3D] {msg}", flush=True)


def make_client(space):
    token = os.environ.get("HF_TOKEN") or None
    return Client(space, token=token, verbose=False)


def text_to_image(prompt, out_path, seed):
    log(f"컨셉 이미지 생성 ({IMAGE_SPACE})")
    client = make_client(IMAGE_SPACE)
    result, used_seed = client.predict(
        prompt=f"{prompt}, {STYLE}",
        seed=max(seed, 0),
        randomize_seed=seed < 0,
        width=1024,
        height=1024,
        num_inference_steps=4,
        api_name="/infer",
    )
    path = result["path"] if isinstance(result, dict) else result
    shutil.copyfile(path, out_path)
    log(f"  -> {out_path} (seed {int(used_seed)})")


def image_to_glb(image_path, out_path, seed, resolution, faces, texture_size):
    log(f"3D 모델 생성 ({MESH_SPACE}, 해상도 {resolution})")
    client = make_client(MESH_SPACE)
    # TRELLIS.2 데모는 세션 단위로 중간 결과를 들고 있어서 같은 클라이언트로 순서대로 불러야 한다
    client.predict(api_name="/start_session")
    image = client.predict(input=handle_file(image_path), api_name="/preprocess_image")
    image_path_pre = image["path"] if isinstance(image, dict) else image
    client.predict(
        image=handle_file(image_path_pre),
        seed=max(seed, 0),
        resolution=str(resolution),
        api_name="/image_to_3d",
    )
    log(f"  GLB 추출 (면 {faces}, 텍스처 {texture_size})")
    glb, _download = client.predict(
        decimation_target=faces,
        texture_size=texture_size,
        api_name="/extract_glb",
    )
    shutil.copyfile(glb, out_path)
    webp_to_png(out_path)
    log(f"  -> {out_path} ({os.path.getsize(out_path) // 1024} KB)")


def webp_to_png(glb_path):
    """TRELLIS.2는 텍스처를 WebP(EXT_texture_webp)로 넣는데 언리얼 Interchange가 못 읽는다. PNG로 바꿔 다시 쓴다."""
    import io
    import json
    import struct

    from PIL import Image

    with open(glb_path, "rb") as f:
        data = f.read()
    magic, version, _length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF":
        return
    offset = 12
    doc, bin_chunk = None, b""
    while offset < len(data):
        clen, ctype = struct.unpack_from("<I4s", data, offset)
        chunk = data[offset + 8:offset + 8 + clen]
        if ctype == b"JSON":
            doc = json.loads(chunk)
        elif ctype == b"BIN\x00":
            bin_chunk = chunk
        offset += 8 + clen
    if doc is None or "EXT_texture_webp" not in doc.get("extensionsUsed", []):
        return

    views = doc["bufferViews"]
    blobs = [bin_chunk[v.get("byteOffset", 0):v.get("byteOffset", 0) + v["byteLength"]] for v in views]
    converted = 0
    for img in doc.get("images", []):
        if img.get("mimeType") == "image/webp" and "bufferView" in img:
            png = io.BytesIO()
            Image.open(io.BytesIO(blobs[img["bufferView"]])).save(png, "PNG", optimize=False)
            blobs[img["bufferView"]] = png.getvalue()
            img["mimeType"] = "image/png"
            converted += 1
    for tex in doc.get("textures", []):
        ext = tex.get("extensions", {})
        if "EXT_texture_webp" in ext:
            tex["source"] = ext.pop("EXT_texture_webp")["source"]
            if not ext:
                tex.pop("extensions")
    for key in ("extensionsUsed", "extensionsRequired"):
        if key in doc:
            doc[key] = [e for e in doc[key] if e != "EXT_texture_webp"]
            if not doc[key]:
                doc.pop(key)

    # 버퍼 뷰를 4바이트 정렬로 다시 이어 붙인다
    new_bin = bytearray()
    for v, blob in zip(views, blobs):
        while len(new_bin) % 4:
            new_bin.append(0)
        v["byteOffset"] = len(new_bin)
        v["byteLength"] = len(blob)
        v["buffer"] = 0
        new_bin += blob
    while len(new_bin) % 4:
        new_bin.append(0)
    doc["buffers"] = [{"byteLength": len(new_bin)}]

    js = json.dumps(doc, separators=(",", ":")).encode("utf-8")
    js += b" " * ((4 - len(js) % 4) % 4)
    out = struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(js) + 8 + len(new_bin))
    out += struct.pack("<I4s", len(js), b"JSON") + js
    out += struct.pack("<I4s", len(new_bin), b"BIN\x00") + bytes(new_bin)
    with open(glb_path, "wb") as f:
        f.write(out)
    log(f"  WebP 텍스처 {converted}개 -> PNG")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True, help="에셋 이름 (예: SM_FloatingShrine)")
    src = parser.add_mutually_exclusive_group()
    src.add_argument("--prompt", help="영어 묘사. 공통 스타일이 자동으로 붙는다")
    src.add_argument("--image", help="이미 있는 컨셉 이미지 경로")
    parser.add_argument("--seed", type=int, default=-1, help="-1이면 무작위")
    parser.add_argument("--resolution", type=int, default=1024, choices=[512, 1024, 1536])
    parser.add_argument("--faces", type=int, default=100000, help="게임용 면 수 목표 (TRELLIS.2 최소 100000, 임포트 때 Nanite가 처리)")
    parser.add_argument("--texture", type=int, default=2048)
    parser.add_argument("--fix-only", action="store_true", help="이미 받은 GLB의 텍스처만 PNG로 바꾼다")
    args = parser.parse_args()

    out_dir = os.path.join(OUT_ROOT, args.name)
    os.makedirs(out_dir, exist_ok=True)
    concept = os.path.join(out_dir, "concept.png")
    glb = os.path.join(out_dir, f"{args.name}.glb")

    if args.fix_only:
        webp_to_png(glb)
        return
    if not args.prompt and not args.image:
        parser.error("--prompt 또는 --image가 필요하다")

    start = time.time()
    if args.prompt:
        with open(os.path.join(out_dir, "prompt.txt"), "w", encoding="utf-8") as f:
            f.write(args.prompt + "\n")
        text_to_image(args.prompt, concept, args.seed)
    elif os.path.abspath(args.image) != os.path.abspath(concept):
        shutil.copyfile(args.image, concept)

    image_to_glb(concept, glb, args.seed, args.resolution, args.faces, args.texture)
    log(f"완료 {time.time() - start:.0f}초")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:  # 할당량 초과 등은 Space가 문자열 에러로 돌려준다
        log(f"실패: {e}")
        sys.exit(1)
