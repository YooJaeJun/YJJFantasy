#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJTraversal.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * 상승 기류. 활공 중에 들어가면 위로 솟구친다. 높은 섬으로 가는 길.
 * 판정은 영웅 쪽에서 IsInside로 한다.
 */
UCLASS()
class YJJFANTASY_API AYJJUpdraft : public AActor
{
	GENERATED_BODY()

public:
	AYJJUpdraft();

	void Setup(float InRadius, float InHeight);
	virtual void Tick(float DeltaTime) override;

	bool IsInside(const FVector& P) const;
	float GetTopZ() const { return GetActorLocation().Z + Height; }

	float Radius = 300.f;
	float Height = 3000.f;

protected:
	virtual void BeginPlay() override;

private:
	// ---- 모양 (cm)
	static constexpr float FloorMarkLift = 5.f;
	static constexpr float FloorMarkThickness = 6.f;
	static constexpr float FloorMarkGlow = 1.2f;
	static constexpr float FloorMarkCore = 0.6f;
	static constexpr float FloorMarkRim = 0.4f;
	static constexpr float ColumnGlow = 0.22f;
	static constexpr int32 RingCount = 7;
	static constexpr float RingWidth = 1.6f;
	static constexpr float RingThickness = 10.f;
	static constexpr float RingGlow = 1.4f;
	static constexpr float RingRiseSpeed = 0.35f;
	static constexpr float RingStartScale = 0.5f;
	static constexpr float RingGrow = 0.7f;
	static constexpr float RingFlat = 0.06f;
	static constexpr int32 StreakCount = 14;
	static constexpr float StreakThickness = 6.f;
	static constexpr float StreakLength = 220.f;
	static constexpr float StreakGlow = 3.f;
	static constexpr float StreakRiseMin = 0.5f;
	static constexpr float StreakRiseRange = 0.4f;
	static constexpr float StreakSwirl = 0.8f;
	static constexpr float StreakRadiusMin = 0.3f;
	static constexpr float StreakRadiusRange = 0.6f;
	static constexpr int32 HashStreak = 13;
	static constexpr int32 HashStreakSalt = 5;
	static constexpr int32 HashStreakRadius = 7;

	float Time = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Rings;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Streaks;
};

/** 튕김 버섯. 밟으면 높이 튀어 오른다 */
UCLASS()
class YJJFANTASY_API AYJJBouncePad : public AActor
{
	GENERATED_BODY()

public:
	AYJJBouncePad();

	void Setup(float InLaunchSpeed, float InSize, const FLinearColor& InCapColor);
	virtual void Tick(float DeltaTime) override;

	float LaunchSpeed = 1800.f;
	float Size = 1.f;
	FLinearColor CapColor = FLinearColor(0.95f, 0.3f, 0.35f);

protected:
	virtual void BeginPlay() override;

private:
	// ---- 모양 (크기 1 기준 cm)
	static constexpr float StemWidth = 80.f;
	static constexpr float StemHeight = 140.f;
	static constexpr float CapPivotHeight = 150.f;
	inline static const FVector CapSize = FVector(300.f, 300.f, 130.f);
	static constexpr float CapSideShade = 0.6f;
	static constexpr float CapEmissive = 0.25f;
	static constexpr int32 DotCount = 7;
	static constexpr int32 DotRings = 3;
	static constexpr float DotRingMin = 70.f;
	static constexpr float DotRingStep = 25.f;
	static constexpr float DotTopHeight = 58.f;
	static constexpr float DotCurve = 2400.f;
	inline static const FVector DotSize = FVector(48.f, 48.f, 22.f);
	static constexpr float DotEmissive = 0.2f;
	static constexpr float GoldenAngle = 2.399963f;
	// ---- 튕길 때 출렁임
	static constexpr float SquashDecay = 6.f;
	static constexpr float WobbleSpeed = 30.f;
	static constexpr float WobbleWiden = 0.25f;
	static constexpr float WobbleSquash = 0.35f;
	static constexpr float PitchMin = 0.95f;
	static constexpr float PitchMax = 1.08f;

	float Squash = 0.f;
	float Cooldown = 0.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> CapPivot;
};

/** 두 지점을 오가는 떠 있는 발판 */
UCLASS()
class YJJFANTASY_API AYJJMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AYJJMovingPlatform();

	void Setup(const FVector& InOffset, float InPeriod, float InRadius, float InPhase);
	virtual void Tick(float DeltaTime) override;

	FVector Offset = FVector(0, 0, 600);
	float Period = 6.f;
	float Radius = 220.f;
	float Phase = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	// ---- 모양 (cm)
	static constexpr float SlabThickness = 60.f;
	static constexpr float RootWidth = 0.9f;
	static constexpr float GemDrop = 1.05f;
	inline static const FVector GemSize = FVector(40.f, 40.f, 80.f);
	static constexpr float GemGlow = 3.f;
	inline static const FRotator UpsideDown = FRotator(180.f, 0.f, 0.f);

	FVector Start = FVector::ZeroVector;
	float Time = 0.f;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;
};
