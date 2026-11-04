# YJJFantasy 도구

엔진: `D:\source\repos\UnrealEngine` (UE 5.8.1 소스 빌드, YJJJump와 같은 엔진)

## 빌드

```
D:\source\repos\UnrealEngine\Engine\Build\BatchFiles\Build.bat YJJFantasyEditor Win64 Development -project="D:\source\repos\YJJFantasy\YJJFantasy.uproject" -WaitMutex
```

## 콘텐츠 (에디터를 끈 상태에서)

```
powershell -File Tools/Content/BuildContent.ps1
```

- `Tools/Audio/generate_audio.py` : 효과음·배경음 합성 (외부 음원 없음)
- `Tools/Content/build_content.py` : 머티리얼(표면·발광·물·셀 셰이딩), 소리·AI 모델 임포트, 빈 맵

월드(섬, 나무, 유적, 등불, 슬라임…)는 에셋이 아니라 코드(`Source/YJJFantasy/World/YJJWorldBuilder.cpp`)가 게임 시작 때 짓는다.

## 실행·테스트

```
powershell -File Tools/RunGame.ps1 -Mode none   # 그냥 플레이
powershell -File Tools/RunGame.ps1 -Mode tour   # 명소 스크린샷 -> Saved/Screenshots/Tour_*.png
powershell -File Tools/RunGame.ps1 -Mode play   # 자동 플레이 검증 -> 로그 [AutoPilot] PASS/FAIL
```

게임 중 콘솔(`~`): `yjj.Tour`, `yjj.AutoPlay`, `yjj.LightAll`, `yjj.Toon <Ref> <Strength> <Outline>`

## AI 3D 모델 (무료)

```
Tools/AI3D/.venv/Scripts/python.exe Tools/AI3D/gen3d.py --name SM_이름 --prompt "영어 묘사"
```

텍스트 → FLUX.1-schnell(컨셉 이미지) → TRELLIS.2(GLB). Hugging Face 무료 GPU(ZeroGPU)를 쓴다.
익명은 하루 할당량이 아주 적다. 무료 Hugging Face 계정의 토큰을 `HF_TOKEN` 환경변수에 넣으면 늘어난다.
생성된 GLB는 `BuildContent.ps1`이 `/Game/YJJFantasy/AI/`로 임포트한다.

## 조작

WASD 이동 · Shift(또는 우클릭) 달리기 · Space 점프 / 공중에서 활공 켜기·끄기 · 좌클릭 공격 · E 상호작용
