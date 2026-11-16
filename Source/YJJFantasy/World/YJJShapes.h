#pragma once

#include "CoreMinimal.h"

class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** 엔진 기본 도형 (모두 100cm 크기, 피벗은 가운데) */
enum class EYJJShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
	Plane,
};

/**
 * 기본 도형 + 스타일라이즈드 머티리얼로 월드를 조립하는 도우미.
 * 섬, 나무, 유적, 캐릭터 몸체가 모두 이걸로 만들어진다.
 */
namespace YJJShapes
{
	UStaticMesh* Mesh(EYJJShape Shape);

	/** 윗면(Top)/옆면(Side) 색이 나뉘는 표면. Emissive > 0 이면 스스로 빛난다 */
	UMaterialInstanceDynamic* Surface(UObject* Outer, const FLinearColor& Top, const FLinearColor& Side, float Emissive = 0.f, float Roughness = 0.85f);

	/** 단색 표면 (Top = Side) */
	UMaterialInstanceDynamic* Solid(UObject* Outer, const FLinearColor& Color, float Emissive = 0.f, float Roughness = 0.85f);

	/** 가산 발광 (빛기둥, 바람, 별가루) */
	UMaterialInstanceDynamic* Glow(UObject* Outer, const FLinearColor& Color, float Intensity, float Core = 0.35f, float Rim = 1.f);

	/** 흐르는 물 */
	UMaterialInstanceDynamic* Water(UObject* Outer, float Speed = 1.f, float Opacity = 0.8f);

	/**
	 * 도형 컴포넌트를 붙인다. Size는 cm 단위 실제 크기, Location/Rotation은 Parent 기준.
	 * bCollide면 플레이어가 밟을 수 있다.
	 */
	UStaticMeshComponent* Add(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, const FVector& Location, const FVector& Size,
		UMaterialInterface* Material, bool bCollide, const FRotator& Rotation = FRotator::ZeroRotator);

	/** AI로 만든 모델 (/Game/YJJFantasy/AI/<Name>/...). 임포트 안 됐으면 nullptr */
	UStaticMesh* AIMesh(const TCHAR* Name);

	/**
	 * AI 모델을 붙인다 (충돌 없음). Size는 가로로 가장 긴 폭(cm).
	 * bBottomPivot이면 Location이 모델 바닥 가운데, 아니면 모델 중심이 된다.
	 */
	UStaticMeshComponent* AddAI(AActor* Owner, USceneComponent* Parent, UStaticMesh* AI, const FVector& Location, float Size,
		bool bBottomPivot, const FRotator& Rotation = FRotator::ZeroRotator);

	/** 같은 도형·머티리얼을 많이 뿌릴 때 (풀, 꽃, 돌멩이) */
	UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, UMaterialInterface* Material, bool bCollide);

	/**
	 * 충돌이 있는 도형을 만들 때마다 이 배열에 담는다 (nullptr이면 안 담음).
	 * 월드를 짓는 같은 프레임에는 물리 바디가 아직 준비되지 않아 레이 검사가 안 되므로,
	 * 월드 빌더가 이 목록으로 도형과 직접 교차 계산을 해서 땅을 찾는다.
	 */
	struct FCollider
	{
		EYJJShape Shape = EYJJShape::Cube;
		FTransform Transform;
	};
	void SetColliderSink(TArray<FCollider>* Sink);

	/** 도형(100cm 기본 도형 + 변환)과 선분의 첫 교차. OutT는 0~1, OutNormal은 월드 법선 */
	bool RayCast(const FCollider& Collider, const FVector& Start, const FVector& End, float& OutT, FVector& OutNormal);

	/** 0~1 사이 결정적 난수 (같은 Seed면 항상 같은 값) */
	float Hash(int32 Seed);

	/** 색을 밝기 배율로 조정 */
	FLinearColor Shade(const FLinearColor& Color, float Scale);
}
