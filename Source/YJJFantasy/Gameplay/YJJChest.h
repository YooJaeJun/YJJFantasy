#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJChest.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 숨겨진 보물 상자. 열면 뚜껑이 젖혀지며 별가루가 쏟아진다.
 * 찾기 어려운 곳(나무 꼭대기, 떠 있는 바위, 탑 뒤)에 있어서 탐험할 이유가 된다.
 */
UCLASS()
class YJJFANTASY_API AYJJChest : public AActor
{
	GENERATED_BODY()

public:
	AYJJChest();

	virtual void Tick(float DeltaTime) override;

	bool CanInteract(const FVector& From) const;
	void Interact();
	bool IsOpened() const { return bOpened; }

	static constexpr int32 StardustInside = 5;
	static constexpr float InteractRange = 230.f;

protected:
	virtual void BeginPlay() override;

private:
	// ---- 모양 (cm)
	inline static const FVector BodySize = FVector(150.f, 100.f, 80.f);
	static constexpr float TrimThickness = 4.f;
	static constexpr float TrimInset = 1.f;
	static constexpr float TrimOver = 6.f;
	static constexpr float LidHeight = 70.f;
	static constexpr float LidBandWidth = 20.f;
	/** 반원기둥 뚜껑을 옆으로 눕힌다 */
	static constexpr float LidRoll = 90.f;
	inline static const FVector LockOffset = FVector(102.f, 0.f, -6.f);
	inline static const FVector LockSize = FVector(8.f, 26.f, 30.f);
	static constexpr float WoodRoughness = 0.7f;
	static constexpr float GoldEmissive = 0.25f;
	static constexpr float GoldRoughness = 0.3f;
	static constexpr float GlowHeight = 70.f;
	inline static const FVector GlowSize = FVector(260.f, 220.f, 180.f);
	static constexpr float GlowIntensity = 1.2f;
	static constexpr float LightHeight = 120.f;
	static constexpr float LightIdle = 60.f;
	static constexpr float LightRadius = 700.f;
	/** 상호작용 거리를 재는 기준 높이 */
	static constexpr float InteractHeight = 60.f;
	/** 별가루가 튀어나오는 높이 */
	static constexpr float PopHeight = 110.f;

	// ---- 움직임 (초, 도)
	static constexpr float WiggleBeat = 2.5f;
	static constexpr float WiggleShake = 17.f;
	static constexpr float WiggleAngle = 6.f;
	static constexpr float GlowIdleBase = 0.7f;
	static constexpr float GlowIdleRange = 0.5f;
	static constexpr float GlowIdleSpeed = 3.f;
	static constexpr float OpenDuration = 0.5f;
	static constexpr float OpenAngle = 105.f;
	static constexpr float OpenOvershoot = 12.f;
	static constexpr float GlowBurst = 3.f;
	static constexpr float GlowBurstTime = 1.5f;
	static constexpr float LightBurst = 300.f;
	static constexpr float LightBurstTime = 2.f;

	bool bOpened = false;
	float Time = 0.f;
	float OpenTime = 0.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Glow;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GlowMID;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;
};
