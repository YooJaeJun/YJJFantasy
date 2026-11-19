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
 * 기본 도형 + 스타일라이즈드 머티리얼로 월드를 조립하는 도우미. 모든 함수는 정적.
 * 섬, 나무, 유적, 소품이 모두 이걸로 만들어진다.
 */
class YJJFANTASY_API YJJShapes
{
public:
	/** 엔진 기본 도형 한 변 (cm) */
	static constexpr float BaseShapeSize = 100.f;
	/** AddFace 의 Mood 종류 수 */
	static constexpr int32 FaceMoodCount = 3;
	static constexpr float DefaultRoughness = 0.85f;

	static UStaticMesh* Mesh(EYJJShape Shape);

	/** "Cube", "Sphere", "Cylinder", "Cone", "Plane" (Placements.csv 의 Text). 모르면 Cube */
	static EYJJShape ShapeFromName(const FString& Name);

	/** 윗면(Top)/옆면(Side) 색이 나뉘는 표면. Emissive > 0 이면 스스로 빛난다 */
	static UMaterialInstanceDynamic* Surface(UObject* Outer, const FLinearColor& Top, const FLinearColor& Side, float Emissive = 0.f, float Roughness = DefaultRoughness);

	/** 단색 표면 (Top = Side) */
	static UMaterialInstanceDynamic* Solid(UObject* Outer, const FLinearColor& Color, float Emissive = 0.f, float Roughness = DefaultRoughness);

	/** 가산 발광 (빛기둥, 바람, 별가루) */
	static UMaterialInstanceDynamic* Glow(UObject* Outer, const FLinearColor& Color, float Intensity, float Core = 0.35f, float Rim = 1.f);

	/** 흐르는 물 */
	static UMaterialInstanceDynamic* Water(UObject* Outer, float Speed = 1.f, float Opacity = 0.8f);

	/**
	 * 도형 컴포넌트를 붙인다. Size는 cm 단위 실제 크기, Location/Rotation은 Parent 기준.
	 * bCollide면 플레이어가 밟을 수 있다.
	 */
	static UStaticMeshComponent* Add(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, const FVector& Location, const FVector& Size,
		UMaterialInterface* Material, bool bCollide, const FRotator& Rotation = FRotator::ZeroRotator);

	/** AI로 만든 모델 (/Game/YJJFantasy/AI/<Name>/...). 임포트 안 됐으면 nullptr */
	static UStaticMesh* AIMesh(const TCHAR* Name);

	/** Blender로 만든 캐릭터용 스태틱 메시 (/Game/YJJFantasy/Characters/<Name>/...). 없으면 nullptr */
	static UStaticMesh* CharacterMesh(const TCHAR* Name);

	/**
	 * AI 모델을 붙인다 (충돌 없음). Size는 가로로 가장 긴 폭(cm).
	 * bBottomPivot이면 Location이 모델 바닥 가운데, 아니면 모델 중심이 된다.
	 */
	static UStaticMeshComponent* AddAI(AActor* Owner, USceneComponent* Parent, UStaticMesh* AI, const FVector& Location, float Size,
		bool bBottomPivot, const FRotator& Rotation = FRotator::ZeroRotator);

	/**
	 * 멍한 만화 얼굴 (점 눈 + ω 입 + 볼). 배경 소품(구름·나무·바위)을 엉뚱하게 만든다.
	 * Location은 얼굴 가운데, Rotation의 +X가 얼굴이 보는 방향, Width는 두 눈 바깥까지 폭.
	 * Mood 0 = 세로 점 눈, 1 = 졸린 가로 눈, 2 = 동그란 놀란 눈
	 */
	static void AddFace(AActor* Owner, USceneComponent* Parent, const FVector& Location, const FRotator& Rotation, float Width, int32 Mood);

	/** 같은 도형·머티리얼을 많이 뿌릴 때 (풀, 꽃, 돌멩이) */
	static UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, UMaterialInterface* Material, bool bCollide);

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
	static void SetColliderSink(TArray<FCollider>* Sink);

	/** 도형(100cm 기본 도형 + 변환)과 선분의 첫 교차. OutT는 0~1, OutNormal은 월드 법선 */
	static bool RayCast(const FCollider& Collider, const FVector& Start, const FVector& End, float& OutT, FVector& OutNormal);

	/** 0~1 사이 결정적 난수 (같은 Seed면 항상 같은 값) */
	static float Hash(int32 Seed);

	/** 색을 밝기 배율로 조정 */
	static FLinearColor Shade(const FLinearColor& Color, float Scale);

private:
	/** 엔진/게임 에셋은 GC 루트에 올려 두고 계속 쓴다 */
	template <typename T>
	static T* LoadCached(const TCHAR* Path)
	{
		if (TWeakObjectPtr<UObject>* Found = AssetCache.Find(Path))
		{
			if (Found->IsValid())
			{
				return Cast<T>(Found->Get());
			}
		}
		T* Loaded = LoadObject<T>(nullptr, Path);
		if (Loaded)
		{
			Loaded->AddToRoot();
			AssetCache.Add(Path, Loaded);
		}
		return Loaded;
	}

	static UMaterialInstanceDynamic* MakeMID(UObject* Outer, const TCHAR* Path);

	static TMap<FString, TWeakObjectPtr<UObject>> AssetCache;
	static TArray<FCollider>* ColliderSink;
};
