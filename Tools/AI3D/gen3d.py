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
    log(f"  -> {out_path} ({os.path.getsize(out_path) // 1024} KB)")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True, help="에셋 이름 (예: SM_FloatingShrine)")
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--prompt", help="영어 묘사. 공통 스타일이 자동으로 붙는다")
    src.add_argument("--image", help="이미 있는 컨셉 이미지 경로")
    parser.add_argument("--seed", type=int, default=-1, help="-1이면 무작위")
    parser.add_argument("--resolution", type=int, default=1024, choices=[512, 1024, 1536])
    parser.add_argument("--faces", type=int, default=60000, help="게임용 면 수 목표")
    parser.add_argument("--texture", type=int, default=2048)
    args = parser.parse_args()

    out_dir = os.path.join(OUT_ROOT, args.name)
    os.makedirs(out_dir, exist_ok=True)
    concept = os.path.join(out_dir, "concept.png")
    glb = os.path.join(out_dir, f"{args.name}.glb")

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
