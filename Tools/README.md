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

월드(섬, 나무, 유적, 등불, 떡무지…)는 에셋이 아니라 코드(`Source/YJJFantasy/World/YJJWorldBuilder.cpp`)가 게임 시작 때 데이터를 읽어 짓는다.

## 데이터·글자 (`Content/YJJFantasy/Data/*.csv`, UTF-8 BOM)

코드에 수치·배치·글자를 박지 않는다. 고치면 게임을 다시 시작하거나 콘솔 `yjj.ReloadData` (섬 배치는 다시 시작해야 반영).

| 파일 | 내용 |
| --- | --- |
| `Localization.csv` | 화면 글자 (Key, ko, en, Note). 언어는 명령줄 `-yjjlang=en` |
| `Tuning.csv` | 이동·기력·공격·체력·카메라·떡무지·별가루·소리 수치 (`FYJJData::Tune`) |
| `Colors.csv` | 색상표, 선형 색 `R\|G\|B` (`FYJJData::Color`) |
| `Islands.csv` | 섬 위치·크기·잔디/바위 색·꽃·구역 이름 키 |
| `Placements.csv` | 섬 위 모든 것. `Type`별로 쓰는 열이 다르다 (Note 열 참고). `Island=World`면 섬 좌표 그대로, `Ground=1`이면 땅 위로. `Terrain`은 풀보다 먼저, `Shape`는 나중 |
| `VistaIslands.csv` | 닿을 수 없는 먼 섬 |
| `Tour.csv` | 투어 스크린샷 지점 |

코드 쪽 창구: `Source/YJJFantasy/Data/YJJData.h` (`FYJJData`), `YJJLoc.h` (`FYJJLoc`). 먼 풍경 생성 상수는 `World/YJJWorldLayout.h`.

## 실행·테스트

```
powershell -File Tools/RunGame.ps1 -Mode none   # 그냥 플레이
powershell -File Tools/RunGame.ps1 -Mode tour   # 명소 스크린샷 -> Saved/Screenshots/Tour_*.png
powershell -File Tools/RunGame.ps1 -Mode play   # 자동 플레이 검증 -> 로그 [AutoPilot] PASS/FAIL
```

게임 중 콘솔(`~`): `yjj.Tour`, `yjj.AutoPlay`, `yjj.LightAll`, `yjj.Toon <Ref> <Strength> <Outline>`, `yjj.ReloadData`

## AI 3D 모델 (무료)

```
Tools/AI3D/.venv/Scripts/python.exe Tools/AI3D/gen3d.py --name SM_이름 --prompt "영어 묘사"
```

텍스트 → FLUX.1-schnell(컨셉 이미지) → TRELLIS.2(GLB). Hugging Face 무료 GPU(ZeroGPU)를 쓴다.
익명은 하루 할당량이 아주 적다. 무료 Hugging Face 계정의 토큰을 `HF_TOKEN` 환경변수에 넣으면 늘어난다.
생성된 GLB는 `BuildContent.ps1`이 `/Game/YJJFantasy/AI/`로 임포트한다.

## 캐릭터 (Blender, 리깅·애니메이션)

그림체: 귀엽지만 엉뚱한 2D 모바일 고양이 게임 느낌의 3D화 (오리지널 캐릭터). 굵은 먹선은 셀 셰이딩 포스트 프로세스가 그린다.

```
"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" -b --factory-startup --python Tools/Character/make_hero.py -- --preview
"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" -b --factory-startup --python Tools/Character/make_mob.py -- --preview
```

- `make_hero.py` : 주인공 고양이 냥용사. 모델 + 뼈 15개 + 애니메이션 10개 -> `Generated/SK_Hero.glb`
- `make_mob.py` : 몬스터 떡무지. 몸/얼굴 따로 -> `SM_Mob_Body.glb`, `SM_Mob_Face.glb` (몸 색은 게임이 입힌다)
- `--preview` : `Generated/preview_*.png` 자세별 렌더. `contact_sheet.py`로 한 장에 모아 본다
- `BuildContent.ps1`이 `/Game/YJJFantasy/Characters/`로 임포트한다

## 게임: 하늘섬의 등불

고도 3km 구름 바다 위 하늘섬. 빛을 잃은 바람의 성소를 되살리려면 다섯 섬의 바람 등불을 밝힌다.

| 섬 | 가는 길 | 볼거리 |
| --- | --- | --- |
| 바람의 성소 (시작) | - | 성소, 떠 있는 수정 |
| 들꽃 언덕 | 동쪽 징검돌 | 꽃밭, 슬라임 셋, 하늘 유적으로 가는 상승 기류 |
| 수정섬 | 북쪽 버섯 → 움직이는 발판 | 빛나는 수정, 수정 아치 |
| 폭포 정원 | 서쪽 상승 기류 → 활공 | 계단식 대지, 하늘 아래로 쏟아지는 폭포 |
| 고목의 숲 | 남쪽 절벽에서 활공 | 거대한 고목, 나선 발판 16단 |
| 하늘 유적 | 들꽃 언덕/고목의 숲 상승 기류 | 5층 신전 탑 |

별가루 118개(15개마다 기력 상한 +20), 숨겨진 보물 상자 6개. 등불이 체크포인트이고, 모두 켜면 성소에서 엔딩 연출.

## 조작

WASD 이동 · Shift(또는 우클릭) 달리기 · Space 점프 / 공중에서 활공 켜기·끄기 · 좌클릭 공격 · E 상호작용
