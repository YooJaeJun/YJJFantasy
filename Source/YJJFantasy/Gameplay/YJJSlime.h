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
	float GetRadius() const { return BodyRadius * Size; }

protected:
	virtual void BeginPlay() override;

private:
	// ---- 모양 (크기 1 기준 cm)
	static constexpr float MeshWidth = 150.f;
	/** glTF 앞쪽(+Y)을 액터 앞(+X)으로 */
	static constexpr float MeshYaw = -90.f;
	inline static const FVector FallbackSize = FVector(130.f, 130.f, 110.f);
	static constexpr float BodyCenterHeight = 55.f;
	/** 맞기·부딪히기 판정 반지름 */
	static constexpr float BodyRadius = 60.f;
	static constexpr float TopShade = 1.15f;
	static constexpr float BodyEmissive = 0.35f;
	static constexpr float BodyRoughness = 0.25f;
	static constexpr float ShadowSize = 120.f;
	static constexpr float ShadowLift = 2.f;
	static constexpr float ShadowFlat = 0.02f;
	static constexpr float ShadowShrink = 0.3f;
	static constexpr float GroundProbeUp = 300.f;
	static constexpr float GroundProbeDown = 600.f;
	static constexpr float WalkableNormalZ = 0.7f;
	static constexpr float ArriveDistance = 10.f;
	static constexpr float TurnSpeed = 8.f;
	static constexpr float HitPitchMin = 0.9f;
	static constexpr float HitPitchMax = 1.15f;

	// ---- 통통 튀기 (한 번 뛰는 시간 중 비율)
	static constexpr float AirFraction = 0.6f;
	static constexpr float HopHeight = 90.f;
	static constexpr float AirThin = 0.12f;
	static constexpr float AirStretch = 0.22f;
	static constexpr float LandWiden = 0.25f;
	static constexpr float LandSquash = 0.3f;
	static constexpr float BreathAmount = 0.03f;
	static constexpr float BreathSpeed = 5.f;
	static constexpr float FlashDecay = 8.f;
	static constexpr float FlashGlow = 4.f;
	static constexpr float ChaseGlow = 0.15f;

	// ---- 터지는 연기
	static constexpr int32 PuffCount = 10;
	static constexpr float PuffSize = 50.f;
	static constexpr float PuffShade = 1.3f;
	static constexpr float PuffCore = 0.6f;
	static constexpr float PuffTime = 0.6f;
	static constexpr float PuffCenterHeight = 60.f;
	static constexpr float PuffSpread = 220.f;
	static constexpr float PuffRiseMin = 0.4f;
	static constexpr float PuffRiseRange = 0.8f;
	static constexpr float PuffStartScale = 0.8f;
	static constexpr float PuffEndScale = 0.1f;
	static constexpr float PuffGlow = 2.5f;
	/** 황금각 (라디안): 연기가 고르게 퍼진다 */
	static constexpr float GoldenAngle = 2.399963f;

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
