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

	/** AI로 만든 모델이 임포트되는 폴더 */
	inline const TCHAR* AIModelDir = TEXT("/Game/YJJFantasy/AI");

	/** HUD 글꼴 (Content 기준). 배민 주아체, SIL OFL 1.1 */
	inline const TCHAR* GameFontFile = TEXT("YJJFantasy/Fonts/Jua-Regular.ttf");
}
