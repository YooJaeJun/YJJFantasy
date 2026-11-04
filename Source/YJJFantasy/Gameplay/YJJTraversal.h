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
	FVector Start = FVector::ZeroVector;
	float Time = 0.f;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;
};
