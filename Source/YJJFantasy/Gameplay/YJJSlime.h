#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJSlime.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * 말랑 슬라임. 자기 섬 안을 통통 튀어 다니다가 영웅을 보면 쫓아온다.
 * 닿으면 영웅이 다치고, 칼에 두 번 맞으면 터지면서 별가루를 떨군다.
 */
UCLASS()
class YJJFANTASY_API AYJJSlime : public AActor
{
	GENERATED_BODY()

public:
	AYJJSlime();

	void Setup(const FLinearColor& InColor, float InHomeRadius, float InSize);
	virtual void Tick(float DeltaTime) override;

	/** 칼에 맞음. 죽으면 true */
	bool TakeHit(const FVector& From);
	bool IsAlive() const { return Health > 0; }
	float GetRadius() const { return 60.f * Size; }

protected:
	virtual void BeginPlay() override;

private:
	bool FindGround(const FVector& At, float& OutZ) const;
	void BuildPuffs();
	void Die();

	FLinearColor Color = FLinearColor(0.45f, 0.85f, 0.55f);
	float HomeRadius = 900.f;
	float Size = 1.f;
	FVector Home = FVector::ZeroVector;
	FVector Target = FVector::ZeroVector;
	FVector KnockVelocity = FVector::ZeroVector;
	float GroundZ = 0.f;
	float HopTime = 0.f;
	float WanderTimer = 0.f;
	float Flash = 0.f;
	float DeathTime = -1.f;
	float Time = 0.f;
	bool bChasing = false;
	int32 Health = 2;

	UPROPERTY()
	TObjectPtr<USceneComponent> BodyPivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shadow;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Puffs;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PuffMID;
};
