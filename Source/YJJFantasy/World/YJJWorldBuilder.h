#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/YJJData.h"
#include "World/YJJShapes.h"
#include "YJJWorldBuilder.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/** 도형을 모아 담는 빈 액터 (섬 하나, 나무 하나 등) */
UCLASS()
class YJJFANTASY_API AYJJProp : public AActor
{
	GENERATED_BODY()

public:
	AYJJProp();
};

/** 영웅 주변을 떠다니는 빛 먼지 (반딧불·민들레 씨앗) */
UCLASS()
class YJJFANTASY_API AYJJMotes : public AActor
{
	GENERATED_BODY()

public:
	AYJJMotes();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FMote
	{
		FVector Offset;
		FVector Drift;
		float Phase;
	};
	TArray<FMote> Motes;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Instances;

	float Time = 0.f;
};

/**
 * 하늘섬 월드 전체를 코드로 짓는다: 중앙 성소 섬 + 등불 섬 다섯 + 길(징검돌·버섯·발판·상승 기류) + 먼 풍경.
 * 게임모드가 시작할 때 하나 스폰한다.
 */
UCLASS()
class YJJFANTASY_API AYJJWorldBuilder : public AActor
{
	GENERATED_BODY()

public:
	AYJJWorldBuilder();

	/** 섬들의 기준 높이 (구름 바다 위 3km) */
	static constexpr float BaseZ = 300000.f;

	/** 섬 좌표 (BaseZ 기준) -> 월드 좌표 */
	static FVector W(float X, float Y, float Z) { return FVector(X, Y, BaseZ + Z); }

	/** 영웅 시작 위치 */
	static FVector GetSpawnLocation();
	static float GetSpawnYaw();

protected:
	virtual void BeginPlay() override;

private:
	// ---- 섬과 장식
	AYJJProp* NewProp(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	AYJJProp* BuildIsland(const FVector& Center, float Radius, const FLinearColor& Grass, const FLinearColor& Rock, int32 Seed, bool bCollide = true, bool bDecorate = true);
	void AddHill(AYJJProp* Island, const FVector& Local, float Radius, float Height, const FLinearColor& Grass, const FLinearColor& Rock);
	void Decorate(AYJJProp* Island, float Radius, const FLinearColor& Grass, int32 Seed, int32 FlowerCount, const TArray<FLinearColor>& FlowerColors);
	void AddTree(const FVector& Base, float Scale, int32 Seed, const FLinearColor& Leaf);
	void AddPine(const FVector& Base, float Scale, const FLinearColor& Leaf);
	void AddRock(const FVector& Base, float Size, int32 Seed, const FLinearColor& Color);
	void AddCrystals(const FVector& Base, float Size, const FLinearColor& Color, int32 Seed, bool bLight);
	void AddPillar(const FVector& Base, float Height, float Radius, bool bBroken, const FLinearColor& Stone);
	void AddArch(const FVector& Base, float Yaw, float Width, float Height, const FLinearColor& Stone);
	void AddWaterfall(const FVector& Top, float Yaw, float Width, float Drop);
	AYJJProp* AddStone(const FVector& Top, float Radius, const FLinearColor& Grass, const FLinearColor& Rock, int32 Seed);
	void AddStardust(const FVector& Location);
	void AddChest(const FVector& Location, float Yaw);
	void AddStardustLine(const FVector& From, const FVector& To, int32 Count, float Arc = 0.f);
	void AddStardustRing(const FVector& Center, float Radius, int32 Count);

	/** 위에서 아래로 쏴서 땅 높이를 찾는다 */
	bool GroundAt(const FVector& XY, float& OutZ, float From = 6000.f) const;
	FVector OnGround(const FVector& XY, float Lift = 0.f) const;

	// ---- 데이터로 짓기 (Content/YJJFantasy/Data/Islands.csv, Placements.csv, VistaIslands.csv)
	void BuildIslands();
	void DecorateIslands();
	/** bEarly 면 언덕·지형(풀보다 먼저), 아니면 나머지 */
	void PlaceAll(bool bEarly);
	void Place(const FYJJPlacementRow& Row);
	FVector PlacementOrigin(const FYJJPlacementRow& Row) const;
	void AddShape(const FYJJPlacementRow& Row, const FVector& Origin);
	void AddRing(const FYJJPlacementRow& Row, const FVector& Center);
	void AddAIShrineIsland(const FVector& Location, float Width, float Yaw);
	void BuildVista();
	static bool IsEarlyPlacement(const FYJJPlacementRow& Row);

	void AddZone(const FString& Name, const FString& Subtitle, const FVector& Center, float Radius);

	template <typename T>
	static T* SpawnDeferred(UWorld* World, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return World->SpawnActorDeferred<T>(T::StaticClass(), FTransform(Rotation, Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}

	template <typename T>
	static void Finish(T* Actor)
	{
		Actor->FinishSpawning(Actor->GetTransform());
	}

	/** 섬 키 -> 섬 바닥 액터 (언덕·지형 조각을 붙인다) */
	UPROPERTY()
	TMap<FName, TObjectPtr<AYJJProp>> IslandProps;

	/** 짓는 동안 만든 충돌 도형 (땅 찾기용) */
	TArray<YJJShapes::FCollider> Colliders;
};
