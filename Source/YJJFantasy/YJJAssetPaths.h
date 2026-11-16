#pragma once

#include "CoreMinimal.h"

/** 코드에서 쓰는 에셋 경로. 머티리얼은 Tools/Content/build_content.py가 만든다. */
namespace YJJAssetPaths
{
	inline const TCHAR* SurfaceMaterial = TEXT("/Game/YJJFantasy/Materials/M_YJJ_Surface.M_YJJ_Surface");
	inline const TCHAR* GlowMaterial = TEXT("/Game/YJJFantasy/Materials/M_YJJ_Glow.M_YJJ_Glow");
	inline const TCHAR* WaterMaterial = TEXT("/Game/YJJFantasy/Materials/M_YJJ_Water.M_YJJ_Water");
	inline const TCHAR* ToonPostProcess = TEXT("/Game/YJJFantasy/Materials/PP_YJJ_Toon.PP_YJJ_Toon");

	inline const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	inline const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	inline const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	inline const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");
	inline const TCHAR* Plane = TEXT("/Engine/BasicShapes/Plane.Plane");

	inline const TCHAR* CloudMaterial = TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst");

	/** 주인공 고양이 (Blender: Tools/Character/make_hero.py). 애니메이션은 HeroAnimDir/SK_Hero<이름> */
	inline const TCHAR* HeroMesh = TEXT("/Game/YJJFantasy/Characters/SK_Hero/SK_Hero/SkeletalMeshes/SK_Hero.SK_Hero");
	inline const TCHAR* HeroAnimDir = TEXT("/Game/YJJFantasy/Characters/SK_Hero/SK_Hero/SkeletalMeshes");

	/** AI로 만든 모델이 임포트되는 폴더 */
	inline const TCHAR* AIModelDir = TEXT("/Game/YJJFantasy/AI");

	/** TRELLIS.2로 만든 떠 있는 성소 섬 (없으면 배치하지 않는다) */
	inline const TCHAR* AIFloatingShrine = TEXT("/Game/YJJFantasy/AI/SM_FloatingShrine/SM_FloatingShrine/StaticMeshes/SM_FloatingShrine.SM_FloatingShrine");

	/** HUD 글꼴 (Content 기준). 배민 주아체, SIL OFL 1.1 */
	inline const TCHAR* GameFontFile = TEXT("YJJFantasy/Fonts/Jua-Regular.ttf");
}
